/* FreeRTOS-Plus-TCP single-task SolidSyslog example for QEMU mps2-an385.
 *
 * S08.03 wired SolidSyslog over a hardcoded TEST_* configuration and
 * exposed the configurable fields as `set <name> <value>` commands over
 * the interactive UART channel (hostname, appname, procid, msgid, msg,
 * host, port, facility, severity). S08.04 swaps the original NullBuffer
 * for the portable SolidSyslogCircularBuffer + SolidSyslogFreeRtosMutex
 * and adds a dedicated FreeRTOS Service task that drains the ring —
 * Log() is now non-blocking, the Service task does the UDP I/O.
 *
 * Static IPv4 (10.0.2.15) on the QEMU slirp network with the host
 * reachable at the slirp gateway 10.0.2.2; Bdd/Targets/Common/BddTargetInteractive
 * runs over qemu -serial stdio (CmsdkUart RX wired into newlib's _read
 * in Bdd/Targets/FreeRtos/Common/Syscalls.c). On link-up the IP-task event
 * hook spawns the interactive task and the service task once; UdpSender
 * drives the SolidSyslogFreeRtosDatagram via the static resolver, so
 * each `send N` line over the UART emits N RFC 5424 datagrams to
 * {10.0.2.2, port=g_port}. */

#include "CmsdkUart.h"
#include "BddTargetEnterpriseId.h"
#include "BddTargetInteractive.h"
#include "BddTargetIps.h"
#include "BddTargetLanguage.h"
#include "BddTargetSwitchConfig.h"
#include "SolidSyslog.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogError.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogFreeRtosDatagram.h"
#include "SolidSyslogFreeRtosMutex.h"
#include "SolidSyslogFreeRtosStaticResolver.h"
#include "SolidSyslogFreeRtosSysUpTime.h"
#include "SolidSyslogFreeRtosTcpStream.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogUdpSender.h"

#include <FreeRTOS.h>
#include <task.h>

#include <FreeRTOS_IP.h>
#include <FreeRTOS_Routing.h>
#include <FreeRTOS_Sockets.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMSDK_UART0_BASE_ADDRESS UINT32_C(0x40004000)

/* IRQ number for the QEMU mps2-an385 LAN9118 Ethernet controller. The
 * upstream Plus-TCP NetworkInterface.c enables ISER for this IRQ but does
 * NOT write IPR — leaving the priority at the reset default of 0, which is
 * numerically more urgent than configMAX_SYSCALL_INTERRUPT_PRIORITY and
 * trips configASSERT the first time the ISR calls a FreeRTOS API. We set
 * IPR explicitly here before FreeRTOS_IPInit_Multi triggers the interface
 * init that flips ISER. */
#define ETHERNET_IRQ_NUMBER 13U

/* NVIC IPR (Interrupt Priority Register) base — one byte per IRQ. */
#define NVIC_IPR_BASE_ADDRESS UINT32_C(0xE000E400)

/* NVIC IPR is 8-bit per IRQ but only the top configPRIO_BITS are
 * implemented; lower (zeroed) bits read back as 0. Derive from the
 * FreeRTOSConfig macros so a kernel-config change can't silently leave
 * IRQ 13 at a higher-than-syscall-safe priority. */
#define ETHERNET_IRQ_PRIORITY ((uint8_t) (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8U - configPRIO_BITS)))

/* Unprivileged mirror of SOLIDSYSLOG_UDP_DEFAULT_PORT (514) for BDD listeners. */
#define BDD_TARGET_UDP_PORT 5514U

/* SolidSyslog_Log allocates two char[SOLIDSYSLOG_MAX_MESSAGE_SIZE] frames
 * (~4 KB) plus formatter storage on its formatter path; BddTargetInteractive
 * adds a SOLIDSYSLOG_MAX_MESSAGE_SIZE fgets line buffer (so a `set msg
 * <body>` line carrying a full path-MTU-class message body lands in one
 * read) plus a same-size HandleSet name buffer and newlib printf (~1 KB).
 * Empirically the task hard-faults at *16 (8 KB) once the SolidSyslog
 * setup runs — newlib printf and the formatter path together exceed that
 * budget. *32 (16 KB) was stable while the line buffer was 256 B; the
 * MAX_LINE_LENGTH bump to 2048 B in slice 6 added ~4 KB peak (line + name
 * frames) so *40 (20 KB) gave ~4 KB headroom. S08.09 adds a TCP path —
 * StreamSender.Connect's address+host-formatter storage and
 * TransmitFramed's prefix formatter consume an extra ~512 B of stack on
 * top of the UDP-only chain — empirically tipping a *40 budget over the
 * edge when the SwitchingSender flips transports under repeated sends.
 * *48 (24 KB) restores ~4 KB headroom. A follow-up will introduce
 * CMake-driven memory scaling once the budget is properly characterised.
 * The task only exists in this single-task example, so heap_4 (96 KB)
 * absorbs it. */
#define INTERACTIVE_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 48U)

/* Static IPv4 wiring matching the QEMU slirp default. 10.0.2.15 is the
 * standard slirp DHCP-allocated guest address; we hardcode it here so no
 * DHCP server is required. The destination address — 10.0.2.2, the slirp
 * gateway routed to the QEMU host — is the listener target driven into
 * the static resolver. */
static const uint8_t TEST_IP_ADDRESS[ipIP_ADDRESS_LENGTH_BYTES]       = {10U, 0U, 2U, 15U};
static const uint8_t TEST_NETMASK[ipIP_ADDRESS_LENGTH_BYTES]          = {255U, 255U, 255U, 0U};
static const uint8_t TEST_GATEWAY[ipIP_ADDRESS_LENGTH_BYTES]          = {10U, 0U, 2U, 2U};
static const uint8_t TEST_DNS[ipIP_ADDRESS_LENGTH_BYTES]              = {10U, 0U, 2U, 3U};
static const uint8_t TEST_MAC[ipMAC_ADDRESS_LENGTH_BYTES]             = {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U};
static const uint8_t TEST_DESTINATION_IPV4[ipIP_ADDRESS_LENGTH_BYTES] = {10U, 0U, 2U, 2U};

/* Mutable walking-skeleton state. Defaults populated at boot; the
 * interactive `set <name> <value>` command rewrites these in-place via
 * OnSet below. Storage sizes match RFC 5424 maxima where applicable
 * (APP-NAME 48, MSGID 32) plus null terminator; MSG matches
 * SOLIDSYSLOG_MAX_MESSAGE_SIZE so a single `set msg <body>` can carry a
 * full path-MTU-class body; g_host fits an IPv4 dotted-quad. g_message
 * holds facility/severity (mutated in place) and the messageId/msg
 * pointers (which target the mutable storage so contents are seen on
 * each Log). */
static char     g_appName[49]                       = "SolidSyslogBddTarget";
static char     g_messageId[33]                     = "example";
static char     g_msg[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = "Hello from FreeRTOS";
static char     g_host[16]                          = "10.0.2.2";
static uint16_t g_port                              = (uint16_t) BDD_TARGET_UDP_PORT;
static uint32_t g_endpointVersion                   = 0U;

static struct SolidSyslogMessage g_message = {
    .facility  = SOLIDSYSLOG_FACILITY_LOCAL0,
    .severity  = SOLIDSYSLOG_SEVERITY_INFO,
    .messageId = g_messageId,
    .msg       = g_msg,
};

/* Plus-TCP requires the network interface descriptor and its endpoint(s)
 * to outlive the IP stack. */
static NetworkInterface_t networkInterface;
static NetworkEndPoint_t  networkEndPoint;

static SolidSyslogFreeRtosStaticResolverStorage resolverStorage;
static SolidSyslogFreeRtosDatagramStorage       datagramStorage;
static SolidSyslogFreeRtosTcpStreamStorage      tcpStreamStorage;
static SolidSyslogStreamSenderStorage           tcpSenderStorage;

/* CircularBuffer + FreeRtosMutex composition for cross-task emission.
 * 8 max-sized messages is comfortably above the 3-message BDD scenarios
 * with headroom for a brief Service drain stall, and ~16 KB of .bss is
 * trivial against the mps2-an385's 16 MB SRAM. */
enum
{
    BDD_TARGET_BUFFER_MESSAGES = 8
};

static SolidSyslogCircularBufferStorage bufferStorage[SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE(BDD_TARGET_BUFFER_MESSAGES)];
static SolidSyslogFreeRtosMutexStorage  mutexStorage;

/* Ensures the interactive task is created exactly once even if the network
 * goes down and back up. */
static BaseType_t interactiveTaskCreated = pdFALSE;

extern NetworkInterface_t* pxMPS2_FillInterfaceDescriptor(BaseType_t xEMACIndex, NetworkInterface_t* pxInterface);

static bool TryUpdateString(char* storage, size_t storageSize, const char* value);
static bool TryParseUInt(const char* value, unsigned long* out);

static uint32_t MmioRead32(uintptr_t address)
{
    // NOLINTNEXTLINE(performance-no-int-to-ptr) -- mapping the CMSDK UART MMIO address into a 32-bit volatile pointer.
    return *(volatile uint32_t*) address;
}

static void MmioWrite32(uintptr_t address, uint32_t value)
{
    // NOLINTNEXTLINE(performance-no-int-to-ptr) -- mapping the CMSDK UART MMIO address into a 32-bit volatile pointer.
    *(volatile uint32_t*) address = value;
}

static void RtosSleep(int milliseconds)
{
    /* Round any non-zero millisecond request up to at least one tick so a
     * sub-tick sleep (e.g. CmsdkUart's 1 ms yield against a 100 Hz tick,
     * where pdMS_TO_TICKS(1) == 0) still blocks the task instead of falling
     * through vTaskDelay(0) and busy-spinning the spin-and-yield loops. */
    TickType_t ticks = pdMS_TO_TICKS((TickType_t) milliseconds);
    if ((milliseconds > 0) && (ticks == 0U))
    {
        ticks = 1U;
    }
    vTaskDelay(ticks);
}

static const CmsdkUartMemoryAccess MMIO_ACCESS = {MmioRead32, MmioWrite32, RtosSleep};

static void SetEthernetIrqPriority(void)
{
    // NOLINTNEXTLINE(performance-no-int-to-ptr) -- writing the NVIC IPR byte for IRQ 13.
    volatile uint8_t* ipr = (volatile uint8_t*) (NVIC_IPR_BASE_ADDRESS + ETHERNET_IRQ_NUMBER);
    *ipr                  = ETHERNET_IRQ_PRIORITY;
}

static void GetHostname(struct SolidSyslogFormatter* formatter)
{
    /* RFC 5424 §6.2.4 walk for this reference target:
     *   1. FQDN              — n/a (no DNS resolver on the FreeRTOS reference)
     *   2. Static IP address — present (queried from the IP-stack below)  ← emit this
     *   3. hostname          — n/a (no integrator-supplied hostname)
     *   4. Dynamic IP        — n/a (no DHCP)
     *   5. NILVALUE          — fallthrough if none of the above are available
     *
     * Source of truth is TEST_IP_ADDRESS, which flows to FreeRTOS-Plus-TCP
     * via FreeRTOS_FillEndPoint at boot. Read it back via the IP-stack so
     * any future slice that swaps the static configuration for DHCP (rung
     * 4) or supplies a hostname (rung 3) doesn't have to re-touch this
     * function — same callback, different rung satisfied by the stack. */
    uint32_t ipAddress = 0U;
    char     ipBuffer[16];
    FreeRTOS_GetEndPointConfiguration(&ipAddress, NULL, NULL, NULL, &networkEndPoint);
    FreeRTOS_inet_ntoa(ipAddress, ipBuffer);
    SolidSyslogFormatter_BoundedString(formatter, ipBuffer, strlen(ipBuffer));
}

static void GetAppName(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_BoundedString(formatter, g_appName, strlen(g_appName));
}

/* No RTC and no time-sync on this reference target — the example models an
 * embedded device that has no concept of wall-clock time. RFC 5424 §6.2.3.1
 * mandates NILVALUE TIMESTAMP in that case, and the timeQuality SD reports
 * tzKnown=0, isSynced=0. SolidSyslogConfig.clock=NULL drops through to the
 * library's NilClock; the resulting all-zero SolidSyslogTimestamp fails
 * TimestampIsValid in Core/Source/SolidSyslog.c and emits "-" on the wire. */
static void ErrorHandler(void* context, enum SolidSyslog_Severity severity, const char* message)
{
    (void) context;
    (void) printf("[solidsyslog] severity=%d %s\n", (int) severity, message);
}

static void GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->tzKnown                  = false;
    timeQuality->isSynced                 = false;
    timeQuality->syncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
}

static void GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    /* SolidSyslogFreeRtosStaticResolver currently ignores the host
     * string and routes via TEST_DESTINATION_IPV4, so g_host is plumbed
     * here for forward-compatibility with the follow-up slice that will
     * teach the resolver to parse dotted-quads. The port reaches the
     * wire via sendto unchanged. */
    SolidSyslogFormatter_BoundedString(endpoint->host, g_host, strlen(g_host));
    endpoint->port = g_port;
}

static uint32_t GetEndpointVersion(void)
{
    return g_endpointVersion;
}

static bool OnSet(const char* name, const char* value)
{
    if (strcmp(name, "appname") == 0)
    {
        return TryUpdateString(g_appName, sizeof(g_appName), value);
    }
    if (strcmp(name, "msgid") == 0)
    {
        return TryUpdateString(g_messageId, sizeof(g_messageId), value);
    }
    if (strcmp(name, "msg") == 0)
    {
        return TryUpdateString(g_msg, sizeof(g_msg), value);
    }
    if (strcmp(name, "host") == 0)
    {
        return TryUpdateString(g_host, sizeof(g_host), value);
    }
    if (strcmp(name, "port") == 0)
    {
        unsigned long parsed = 0U;
        if (!TryParseUInt(value, &parsed) || parsed == 0U || parsed > UINT16_MAX)
        {
            return false;
        }
        g_port = (uint16_t) parsed;
        g_endpointVersion++;
        return true;
    }
    if (strcmp(name, "facility") == 0)
    {
        /* Mirrors the Linux example's atoi-and-cast (Bdd/Targets/Common/
         * BddTargetCommandLine.c): the example forwards the parsed value
         * unchanged so the library is the single authority on what's
         * valid (out-of-range facility/severity encodes as the
         * internal-error PRIVAL 43). */
        unsigned long parsed = 0U;
        if (!TryParseUInt(value, &parsed))
        {
            return false;
        }
        g_message.facility = (enum SolidSyslog_Facility) parsed;
        return true;
    }
    if (strcmp(name, "severity") == 0)
    {
        unsigned long parsed = 0U;
        if (!TryParseUInt(value, &parsed))
        {
            return false;
        }
        g_message.severity = (enum SolidSyslog_Severity) parsed;
        return true;
    }
    if (strcmp(name, "transport") == 0)
    {
        /* Switching sender selector — "udp" / "tcp" / "tls" / "mtls" route
         * through BddTargetSwitchConfig (Bdd/Targets/Common). The FreeRTOS
         * target only wires UDP and TCP inner senders today; selecting tls
         * falls through to the SwitchingSender's nil sender, surfacing the
         * gap as a send failure rather than a crash. */
        BddTargetSwitchConfig_SetByName(value);
        return true;
    }
    return false;
}

static bool TryUpdateString(char* storage, size_t storageSize, const char* value)
{
    size_t length = strlen(value);
    if ((length == 0U) || (length >= storageSize))
    {
        return false;
    }
    memcpy(storage, value, length);
    storage[length] = '\0';
    return true;
}

static bool TryParseUInt(const char* value, unsigned long* out)
{
    if (*value == '\0')
    {
        return false;
    }
    char*         end    = NULL;
    unsigned long parsed = strtoul(value, &end, 10);
    /* strtoul accepts a leading '-' and wraps to a huge unsigned. The port
     * call site bounds-checks against UINT16_MAX upstream; facility and
     * severity intentionally don't, mirroring the Linux example's
     * atoi-and-cast so wrapped values reach the library and encode as
     * the internal-error PRIVAL. */
    if (*end != '\0')
    {
        return false;
    }
    *out = parsed;
    return true;
}

static void InteractiveTask(void* argument)
{
    (void) argument;

    struct SolidSyslogResolver* resolver = SolidSyslogFreeRtosStaticResolver_Create(&resolverStorage, TEST_DESTINATION_IPV4);
    struct SolidSyslogDatagram* datagram = SolidSyslogFreeRtosDatagram_Create(&datagramStorage);

    struct SolidSyslogUdpSenderConfig udpConfig = {
        .resolver        = resolver,
        .datagram        = datagram,
        .endpoint        = GetEndpoint,
        .endpointVersion = GetEndpointVersion,
    };
    struct SolidSyslogSender* udpSender = SolidSyslogUdpSender_Create(&udpConfig);

    /* Plain TCP path via the new FreeRTOS Plus-TCP stream adapter. Shares the
     * UDP endpoint callbacks because the BDD oracle (syslog-ng) listens on the
     * same host:port for both transports — the syslog-ng config in
     * Bdd/syslog-ng/syslog-ng.conf has a TCP listener on 5514 alongside UDP. */
    struct SolidSyslogStream*            stream    = SolidSyslogFreeRtosTcpStream_Create(&tcpStreamStorage);
    struct SolidSyslogStreamSenderConfig tcpConfig = {
        .resolver        = resolver,
        .stream          = stream,
        .endpoint        = GetEndpoint,
        .endpointVersion = GetEndpointVersion,
    };
    struct SolidSyslogSender* tcpSender = SolidSyslogStreamSender_Create(&tcpSenderStorage, &tcpConfig);

    /* SwitchingSender lets `set transport <udp|tcp>` flip the active transport
     * at runtime. Default to UDP so existing UDP-tagged scenarios stay green;
     * `--transport tcp` flowing through the behave harness lands here as
     * `set transport tcp` over the UART and switches before the first send. */
    static struct SolidSyslogSender* inners[2];
    inners[BDD_TARGET_SWITCH_UDP]                        = udpSender;
    inners[BDD_TARGET_SWITCH_TCP]                        = tcpSender;
    struct SolidSyslogSwitchingSenderConfig switchConfig = {
        .senders     = inners,
        .senderCount = sizeof(inners) / sizeof(inners[0]),
        .selector    = BddTargetSwitchConfig_Selector,
    };
    BddTargetSwitchConfig_SetByName("udp");
    struct SolidSyslogSender* sender = SolidSyslogSwitchingSender_Create(&switchConfig);

    /* CircularBuffer drained by ServiceTask below, with a FreeRtosMutex
     * gating concurrent producers (interactive task today; multi-task
     * emission in S08.04 slice 3 will add more). The buffer's Read side
     * is the Service task; its Write side is whichever task calls
     * SolidSyslog_Log. */
    struct SolidSyslogMutex*  mutex  = SolidSyslogFreeRtosMutex_Create(&mutexStorage);
    struct SolidSyslogBuffer* buffer = SolidSyslogCircularBuffer_Create(bufferStorage, sizeof(bufferStorage), mutex);
    struct SolidSyslogStore*  store  = SolidSyslogNullStore_Create();

    struct SolidSyslogAtomicCounter* counter    = SolidSyslogAtomicCounter_Create();
    struct SolidSyslogMetaSdConfig   metaConfig = {
          .counter      = counter,
          .getSysUpTime = SolidSyslogFreeRtosSysUpTime_Get,
          .getLanguage  = BddTargetLanguage_Get,
    };
    struct SolidSyslogStructuredData* metaSd        = SolidSyslogMetaSd_Create(&metaConfig);
    struct SolidSyslogStructuredData* timeQualitySd = SolidSyslogTimeQualitySd_Create(GetTimeQuality);
    struct SolidSyslogOriginSdConfig  originConfig  = {
          .software     = "SolidSyslogBddTarget",
          .swVersion    = "0.7.0",
          .enterpriseId = BDD_TARGET_ENTERPRISE_ID,
          .getIpCount   = BddTargetIps_Count,
          .getIpAt      = BddTargetIps_At,
    };
    struct SolidSyslogStructuredData* originSd = SolidSyslogOriginSd_Create(&originConfig);
    struct SolidSyslogStructuredData* sdList[] = {metaSd, timeQualitySd, originSd};

    struct SolidSyslogConfig config = {
        .buffer      = buffer,
        .sender      = sender,
        .clock       = NULL,
        .getHostname = GetHostname,
        .getAppName  = GetAppName,
        /* PROCID — RFC 5424 §6.2.6 NILVALUE: FreeRTOS QEMU has no
         * process model. NULL drops through to the library's
         * NilStringFunction which yields an empty field; FormatStringField
         * (Core/Source/SolidSyslog.c) then emits "-" on the wire. */
        .getProcessId = NULL,
        .store        = store,
        .sd           = sdList,
        .sdCount      = sizeof(sdList) / sizeof(sdList[0]),
    };
    SolidSyslog_SetErrorHandler(ErrorHandler, NULL);
    SolidSyslog_Create(&config);

    BddTargetInteractive_Run(&g_message, stdin, NULL, OnSet);

    SolidSyslog_Destroy();
    SolidSyslogOriginSd_Destroy();
    SolidSyslogTimeQualitySd_Destroy();
    SolidSyslogMetaSd_Destroy();
    SolidSyslogAtomicCounter_Destroy();
    SolidSyslogNullStore_Destroy();
    SolidSyslogCircularBuffer_Destroy(buffer);
    SolidSyslogFreeRtosMutex_Destroy(mutex);
    SolidSyslogSwitchingSender_Destroy();
    SolidSyslogStreamSender_Destroy(tcpSender);
    SolidSyslogFreeRtosTcpStream_Destroy(stream);
    SolidSyslogUdpSender_Destroy();
    SolidSyslogFreeRtosDatagram_Destroy(datagram);
    SolidSyslogFreeRtosStaticResolver_Destroy(resolver);

    vTaskDelete(NULL);
}

/* Drains the CircularBuffer in the background while the interactive task
 * (and, in S08.04 slice 3, additional worker tasks) call SolidSyslog_Log.
 * Equal priority to the producers — FreeRTOS round-robins time slices
 * between same-priority ready tasks so the buffer is drained without the
 * Service task starving slower producers. */
#define SERVICE_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 16U)

static void ServiceTask(void* argument)
{
    (void) argument;
    for (;;)
    {
        SolidSyslog_Service();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void vApplicationIPNetworkEventHook_Multi(eIPCallbackEvent_t eNetworkEvent, struct xNetworkEndPoint* pxEndPoint)
{
    (void) pxEndPoint;
    if ((eNetworkEvent == eNetworkUp) && (interactiveTaskCreated == pdFALSE))
    {
        if (xTaskCreate(InteractiveTask, "interactive", INTERACTIVE_TASK_STACK_DEPTH, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS)
        {
            (void) xTaskCreate(ServiceTask, "service", SERVICE_TASK_STACK_DEPTH, NULL, tskIDLE_PRIORITY + 1, NULL);
            interactiveTaskCreated = pdTRUE;
        }
    }
}

int main(void)
{
    CmsdkUart_Init(&MMIO_ACCESS, CMSDK_UART0_BASE_ADDRESS);

    SetEthernetIrqPriority();

    (void) pxMPS2_FillInterfaceDescriptor(0, &networkInterface);
    FreeRTOS_FillEndPoint(&networkInterface, &networkEndPoint, TEST_IP_ADDRESS, TEST_NETMASK, TEST_GATEWAY, TEST_DNS, TEST_MAC);

    if (FreeRTOS_IPInit_Multi() != pdPASS)
    {
        for (;;)
        {
        }
    }

    vTaskStartScheduler();

    for (;;)
    {
    }
    return 0;
}

void vApplicationMallocFailedHook(void)
{
    for (;;)
    {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char* taskName)
{
    (void) task;
    (void) taskName;
    for (;;)
    {
    }
}

/* Plus-TCP requires the application to provide a per-endpoint random seed
 * source for ARP / IP-ID generation. The QEMU mps2-an385 has no RNG; for a
 * deterministic smoke test we return the run-time tick mixed with a fixed
 * constant. */
BaseType_t xApplicationGetRandomNumber(uint32_t* pulValue)
{
    *pulValue = (uint32_t) xTaskGetTickCount() ^ 0xA5A5A5A5U;
    return pdPASS;
}

uint32_t ulApplicationGetNextSequenceNumber(uint32_t ulSourceAddress, uint16_t usSourcePort, uint32_t ulDestinationAddress, uint16_t usDestinationPort)
{
    (void) ulSourceAddress;
    (void) usSourcePort;
    (void) ulDestinationAddress;
    (void) usDestinationPort;
    return (uint32_t) xTaskGetTickCount();
}
