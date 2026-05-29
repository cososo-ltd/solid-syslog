/* FreeRTOS + lwIP (Raw API, NO_SYS=0) SolidSyslog BDD target for QEMU
 * mps2-an385.
 *
 * S28.09: the worked NO_SYS=0 integration the lwIP guide promises. lwIP runs
 * its own tcpip thread (tcpip_init); a hand-written LAN9118 netif
 * (netif/EthernetIf.c) drives the wire; the SolidSyslog LwipRaw adapters reach
 * the lwIP core through the tcpip_callback marshal (S28.06,
 * SolidSyslogLwipRaw_SetMarshal). The pipeline mirrors the FreeRTOS-Plus-TCP
 * target (Bdd/Targets/FreeRtos/main.c) with the network backend swapped
 * PlusTcp -> LwipRaw: a CircularBuffer + FreeRtosMutex feed a Service task that
 * drains over UDP, and BddTargetInteractive drives `send N` / `set <k> <v>` /
 * `quit` over the QEMU -serial stdio UART.
 *
 * Scope is UDP (S28.09) + TCP (S28.10). The SwitchingSender carries a real UDP
 * sender (LwipRawDatagram) and a real octet-framed TCP sender (StreamSender over
 * LwipRawTcpStream); the TLS slot still resolves to the shared NullSender (drops
 * on the floor) until S28.11 wires the LwipRaw TLS sender.
 *
 * Static IPv4 (10.0.2.15) on the QEMU slirp network, host reachable at the
 * slirp gateway 10.0.2.2 — numeric, because slirp has no route to the docker
 * DNS alias the Linux target uses. */

#include "CmsdkUart.h"
#include "EthernetIf.h"

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
#include "SolidSyslogFreeRtosMutex.h"
#include "SolidSyslogFreeRtosSysUpTime.h"
#include "SolidSyslogLwipRawAddress.h"
#include "SolidSyslogLwipRawDatagram.h"
#include "SolidSyslogLwipRawMarshal.h"
#include "SolidSyslogLwipRawResolver.h"
#include "SolidSyslogLwipRawTcpStream.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStdAtomicCounter.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogUdpSender.h"

#include "lwip/etharp.h"
#include "lwip/init.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"

#include <FreeRTOS.h>
#include <task.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMSDK_UART0_BASE_ADDRESS UINT32_C(0x40004000)

/* Unprivileged mirror of SOLIDSYSLOG_UDP_DEFAULT_PORT (514) for BDD listeners. */
#define BDD_TARGET_UDP_PORT 5514U

/* UDP-only interactive task: BddTargetInteractive's 2048-byte line + name
 * frames plus SolidSyslog_Log's two SOLIDSYSLOG_MAX_MESSAGE_SIZE formatter
 * frames and newlib printf. *40 (20 KB) matches the empirical pre-TLS budget
 * the +TCP target recorded for the same 2048-byte line buffer. heap_4 (96 KB)
 * absorbs it alongside the lwIP tcpip / RX tasks. */
#define INTERACTIVE_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 40U)
#define SERVICE_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 16U)

static char appName[49] = "SolidSyslogBddTarget";
static char messageId[33] = "example";
static char msg[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = "Hello from FreeRTOS lwIP";
static char host[16] = "10.0.2.2";
static uint16_t port = (uint16_t) BDD_TARGET_UDP_PORT;
static uint32_t endpointVersion = 0U;

static struct SolidSyslogMessage testMessage = {
    .Facility = SOLIDSYSLOG_FACILITY_LOCAL0,
    .Severity = SOLIDSYSLOG_SEVERITY_INFORMATIONAL,
    .MessageId = messageId,
    .Msg = msg,
};

/* lwIP netif descriptor — must outlive the tcpip thread. */
static struct netif networkInterface;
/* Gateway IP, kept at file scope so the ARP warm-up (which resolves it before
 * the first datagram) can reach it after bring-up. */
static ip4_addr_t gatewayAddress;

/* CircularBuffer + FreeRtosMutex for cross-task emission. 8 max-sized messages
 * is comfortably above the 3-message BDD scenarios. */
enum
{
    BDD_TARGET_BUFFER_MESSAGES = 8
};

static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(BDD_TARGET_BUFFER_MESSAGES)];

/* Lifecycle mutex serialises SolidSyslog_Service against the teardown path.
 * solidSyslogTeardown is set inside the critical section so Service observes
 * it atomically with the destroy and self-deletes before the mutex goes. */
static struct SolidSyslogMutex* lifecycleMutex = NULL;
static struct SolidSyslog* solidSyslog = NULL;
static volatile bool solidSyslogReady = false;
static volatile bool solidSyslogTeardown = false;

static struct SolidSyslogConfig solidSyslogConfig;
static struct SolidSyslogStructuredData* sdList[3];
static struct SolidSyslogAtomicCounter* atomicCounter = NULL;
static struct SolidSyslogStructuredData* metaSd = NULL;
static struct SolidSyslogStructuredData* timeQualitySd = NULL;
static struct SolidSyslogStructuredData* originSd = NULL;

static struct SolidSyslogResolver* resolver = NULL;
static struct SolidSyslogDatagram* datagram = NULL;
static struct SolidSyslogAddress* udpAddress = NULL;
static struct SolidSyslogSender* udpSender = NULL;
static struct SolidSyslogStream* tcpStream = NULL;
static struct SolidSyslogAddress* tcpAddress = NULL;
static struct SolidSyslogSender* tcpSender = NULL;
static struct SolidSyslogSender* switchingSender = NULL;
static struct SolidSyslogBuffer* buffer = NULL;
static struct SolidSyslogMutex* bufferMutex = NULL;

static TaskHandle_t serviceTaskHandle = NULL;

static void InteractiveTask(void* argument);
static void ServiceTask(void* argument);
static void LwipTcpipMarshal(SolidSyslogLwipRawCallback callback, void* context);
static void NetworkBringUp(void* context);
static void WarmUpGatewayArp(void);
static void GatewayResolvedQuery(void* context);
static bool TryUpdateString(char* storage, size_t storageSize, const char* value);
static bool TryParseUInt(const char* value, unsigned long* out);
static void TeardownAll(void);
static void SemihostingExit(int status);

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
    TickType_t ticks = pdMS_TO_TICKS((TickType_t) milliseconds);
    if ((milliseconds > 0) && (ticks == 0U))
    {
        ticks = 1U;
    }
    vTaskDelay(ticks);
}

static const CmsdkUartMemoryAccess MMIO_ACCESS = {MmioRead32, MmioWrite32, RtosSleep};

/* lwIP randomness source (declared by arch/cc.h's LWIP_RAND). sys_now() is
 * provided by the contrib FreeRTOS sys_arch under NO_SYS=0, so — unlike the
 * S28.07 probe — we do not define it here. A self-contained xorshift32 keeps
 * TCP ISN selection deterministic without newlib rand() / a real entropy
 * backend; adequate for the BDD smoke test. */
unsigned int LwipPortRand(void)
{
    static uint32_t state = 0x2545F491U;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (unsigned int) state;
}

int main(void)
{
    CmsdkUart_Init(&MMIO_ACCESS, CMSDK_UART0_BASE_ADDRESS);

    /* Pin every LwipRaw adapter call onto the tcpip thread. */
    SolidSyslogLwipRaw_SetMarshal(LwipTcpipMarshal);

    /* Create the tcpip thread + mbox + core-lock mutex. Pre-scheduler safe
     * (xTaskCreate / xQueueCreate / xSemaphoreCreate all work before
     * vTaskStartScheduler); the thread itself only runs once the scheduler
     * starts. The netif bring-up is deferred to NetworkBringUp on that thread
     * — smsc9220_init() calls vTaskDelay, which would deref a NULL
     * pxCurrentTCB if run before the scheduler. */
    tcpip_init(NULL, NULL);

    if (xTaskCreate(InteractiveTask, "interactive", INTERACTIVE_TASK_STACK_DEPTH, NULL, tskIDLE_PRIORITY + 1, NULL) !=
        pdPASS)
    {
        SemihostingExit(1);
    }
    if (xTaskCreate(ServiceTask, "service", SERVICE_TASK_STACK_DEPTH, NULL, tskIDLE_PRIORITY + 1, &serviceTaskHandle) !=
        pdPASS)
    {
        SemihostingExit(1);
    }

    vTaskStartScheduler();

    for (;;)
    {
    }
    return 0;
}

/* Runs on the tcpip thread (dispatched via tcpip_callback once the scheduler
 * is up) so netif_add, the link/up transitions, and smsc9220_init's vTaskDelay
 * all execute in a valid task context with the lwIP core lock held. */
static void NetworkBringUp(void* context)
{
    (void) context;
    ip4_addr_t ipAddress;
    ip4_addr_t netmask;
    /* QEMU slirp default: 10.0.2.15 guest, 10.0.2.2 gateway (NATed to the
     * QEMU host, where the syslog-ng oracle listens). */
    IP4_ADDR(&ipAddress, 10, 0, 2, 15);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gatewayAddress, 10, 0, 2, 2);

    (void) netif_add(&networkInterface, &ipAddress, &netmask, &gatewayAddress, NULL, EthernetIf_Init, tcpip_input);
    netif_set_default(&networkInterface);
    netif_set_up(&networkInterface);
    netif_set_link_up(&networkInterface);

    /* Kick off ARP resolution for the gateway now, so the cache is warm before
     * the first datagram. SolidSyslogLwipRawDatagram sends PBUF_REF packets
     * pointing at a transient buffer; if the first send hit an ARP miss the
     * queued copy would reference freed memory and be lost (the documented
     * first-packet drop). The reply is processed by this tcpip thread; the
     * interactive task waits for the cache to populate via WarmUpGatewayArp. */
    (void) etharp_request(&networkInterface, &gatewayAddress);
}

/* Marshalled onto the tcpip thread: report whether the gateway's MAC is in the
 * ARP cache yet. The bool* context is set to the result. */
static void GatewayResolvedQuery(void* context)
{
    struct eth_addr* ethRet = NULL;
    const ip4_addr_t* ipRet = NULL;
    *(bool*) context = (etharp_find_addr(&networkInterface, &gatewayAddress, &ethRet, &ipRet) >= 0);
}

/* Blocks the calling (interactive) task — never the tcpip thread, which must
 * stay free to process the ARP reply — until the gateway resolves or a bounded
 * deadline passes. Generous deadline: QEMU is markedly slower than host. */
static void WarmUpGatewayArp(void)
{
    enum
    {
        WARM_UP_ATTEMPTS = 60,
        WARM_UP_INTERVAL_MS = 50
    };

    for (int attempt = 0; attempt < WARM_UP_ATTEMPTS; attempt++)
    {
        bool resolved = false;
        /* Same synchronous marshal the LwipRaw adapters use: the query runs under
         * the core lock and writes `resolved` before this returns, so there is no
         * race between the queued callback and the next loop iteration. */
        LwipTcpipMarshal(GatewayResolvedQuery, &resolved);
        if (resolved)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(WARM_UP_INTERVAL_MS));
    }
}

static void LwipTcpipMarshal(SolidSyslogLwipRawCallback callback, void* context)
{
    /* The synchronous-marshal contract (SolidSyslogLwipRawMarshal.h) requires the
     * callback's results to be ready when this returns. lwIP's tcpip_callback only
     * blocks until the work is *queued*, so it cannot satisfy that by itself.
     * LWIP_TCPIP_CORE_LOCKING is enabled, so we run the callback in the caller's
     * own task context under the core lock instead: unconditionally synchronous,
     * independent of task priority, and no per-send mailbox message. The lock is
     * recursive and our callbacks never re-marshal, so this cannot self-deadlock. */
    LOCK_TCPIP_CORE();
    callback(context);
    UNLOCK_TCPIP_CORE();
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

static void GetHostname(struct SolidSyslogFormatter* formatter)
{
    /* RFC 5424 section 6.2.4 rung 2 (static IP address) — read back from the
     * netif so a future DHCP slice satisfies the same rung without touching
     * this callback. */
    const char* address = ip4addr_ntoa(netif_ip4_addr(&networkInterface));
    SolidSyslogFormatter_BoundedString(formatter, address, strlen(address));
}

static void GetAppName(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_BoundedString(formatter, appName, strlen(appName));
}

static void GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->TzKnown = false;
    timeQuality->IsSynced = false;
    timeQuality->SyncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
}

static void GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->Host, host, strlen(host));
    endpoint->Port = port;
}

static uint32_t GetEndpointVersion(void)
{
    return endpointVersion;
}

static void ErrorHandlerEx(
    void* context,
    enum SolidSyslogSeverity severity,
    const struct SolidSyslogErrorSource* source,
    uint8_t code
)
{
    (void) context;
    const char* sourceName = "<unknown>";
    const char* message = "<no translation>";
    if (source != NULL)
    {
        sourceName = source->Name;
        if (source->AsString != NULL)
        {
            message = source->AsString(code);
        }
    }
    (void) printf("[solidsyslog] severity=%d [%s/%u] %s\n", (int) severity, sourceName, (unsigned) code, message);
}

static bool OnSet(const char* name, const char* value)
{
    bool handled = false;
    if (strcmp(name, "appname") == 0)
    {
        handled = TryUpdateString(appName, sizeof(appName), value);
    }
    else if (strcmp(name, "msgid") == 0)
    {
        handled = TryUpdateString(messageId, sizeof(messageId), value);
    }
    else if (strcmp(name, "msg") == 0)
    {
        handled = TryUpdateString(msg, sizeof(msg), value);
    }
    else if (strcmp(name, "host") == 0)
    {
        handled = TryUpdateString(host, sizeof(host), value);
        if (handled)
        {
            endpointVersion++;
        }
    }
    else if (strcmp(name, "port") == 0)
    {
        unsigned long parsed = 0U;
        if (TryParseUInt(value, &parsed) && (parsed != 0U) && (parsed <= UINT16_MAX))
        {
            port = (uint16_t) parsed;
            endpointVersion++;
            handled = true;
        }
    }
    else if (strcmp(name, "facility") == 0)
    {
        unsigned long parsed = 0U;
        if (TryParseUInt(value, &parsed))
        {
            testMessage.Facility = (enum SolidSyslogFacility) parsed;
            handled = true;
        }
    }
    else if (strcmp(name, "severity") == 0)
    {
        unsigned long parsed = 0U;
        if (TryParseUInt(value, &parsed))
        {
            testMessage.Severity = (enum SolidSyslogSeverity) parsed;
            handled = true;
        }
    }
    else if (strcmp(name, "transport") == 0)
    {
        BddTargetSwitchConfig_SetByName(value);
        handled = true;
    }
    else if (strcmp(name, "shutdown") == 0)
    {
        unsigned long parsed = 0U;
        if (TryParseUInt(value, &parsed))
        {
            if (parsed != 0U)
            {
                TeardownAll();
                SemihostingExit(0);
            }
            handled = true;
        }
    }
    return handled;
}

static bool TryUpdateString(char* storage, size_t storageSize, const char* value)
{
    bool updated = false;
    size_t length = strlen(value);
    if ((length != 0U) && (length < storageSize))
    {
        memcpy(storage, value, length);
        storage[length] = '\0';
        updated = true;
    }
    return updated;
}

static bool TryParseUInt(const char* value, unsigned long* out)
{
    bool parsed = false;
    if (*value != '\0')
    {
        char* end = NULL;
        unsigned long candidate = strtoul(value, &end, 10);
        if (*end == '\0')
        {
            *out = candidate;
            parsed = true;
        }
    }
    return parsed;
}

static void InteractiveTask(void* argument)
{
    (void) argument;

    /* Bring the netif up on the tcpip thread now the scheduler is running.
     * Blocking callback — returns once the interface is added and link/up. */
    (void) tcpip_callback(NetworkBringUp, NULL);

    /* Wait for the gateway ARP to resolve before any datagram goes out, so the
     * first send is not lost to a cache miss (see WarmUpGatewayArp). */
    WarmUpGatewayArp();

    resolver = SolidSyslogLwipRawResolver_Create();
    datagram = SolidSyslogLwipRawDatagram_Create();
    udpAddress = SolidSyslogLwipRawAddress_Create();

    struct SolidSyslogUdpSenderConfig udpConfig = {
        .Resolver = resolver,
        .Datagram = datagram,
        .Address = udpAddress,
        .Endpoint = GetEndpoint,
        .EndpointVersion = GetEndpointVersion,
    };
    udpSender = SolidSyslogUdpSender_Create(&udpConfig);

    /* Plain TCP path: RFC 6587 octet-framed StreamSender over the LwipRaw TCP
     * stream adapter. Shares the resolver and the UDP endpoint callbacks because
     * the syslog-ng oracle listens on the same host:port for both transports.
     * RtosSleep drives the stream's bounded synchronous-connect spin; the
     * connect timeout comes from the SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable
     * (GetConnectTimeoutMs NULL). */
    struct SolidSyslogLwipRawTcpStreamConfig tcpStreamConfig = {
        .GetConnectTimeoutMs = NULL,
        .ConnectTimeoutContext = NULL,
        .Sleep = RtosSleep,
    };
    tcpStream = SolidSyslogLwipRawTcpStream_Create(&tcpStreamConfig);
    tcpAddress = SolidSyslogLwipRawAddress_Create();
    struct SolidSyslogStreamSenderConfig tcpConfig = {
        .Resolver = resolver,
        .Stream = tcpStream,
        .Address = tcpAddress,
        .Endpoint = GetEndpoint,
        .EndpointVersion = GetEndpointVersion,
    };
    tcpSender = SolidSyslogStreamSender_Create(&tcpConfig);

    /* SwitchingSender lets `set transport <udp|tcp|tls>` flip transport at
     * runtime. UDP and TCP are wired; TLS routes to the shared NullSender (drop
     * on the floor) until S28.11 wires the LwipRaw TLS sender. */
    static struct SolidSyslogSender* inners[BDD_TARGET_SWITCH_COUNT];
    inners[BDD_TARGET_SWITCH_UDP] = udpSender;
    inners[BDD_TARGET_SWITCH_TCP] = tcpSender;
    inners[BDD_TARGET_SWITCH_TLS] = SolidSyslogNullSender_Get();
    struct SolidSyslogSwitchingSenderConfig switchConfig = {
        .Senders = inners,
        .SenderCount = BDD_TARGET_SWITCH_COUNT,
        .Selector = BddTargetSwitchConfig_Selector,
    };
    BddTargetSwitchConfig_SetByName("udp");
    switchingSender = SolidSyslogSwitchingSender_Create(&switchConfig);

    bufferMutex = SolidSyslogFreeRtosMutex_Create();
    buffer = SolidSyslogCircularBuffer_Create(bufferMutex, bufferRing, sizeof(bufferRing));
    lifecycleMutex = SolidSyslogFreeRtosMutex_Create();

    atomicCounter = SolidSyslogStdAtomicCounter_Create();
    struct SolidSyslogMetaSdConfig metaConfig = {
        .Counter = atomicCounter,
        .GetSysUpTime = SolidSyslogFreeRtosSysUpTime_Get,
        .GetLanguage = BddTargetLanguage_Get,
    };
    metaSd = SolidSyslogMetaSd_Create(&metaConfig);
    timeQualitySd = SolidSyslogTimeQualitySd_Create(GetTimeQuality);
    struct SolidSyslogOriginSdConfig originConfig = {
        .Software = "SolidSyslogBddTarget",
        .SwVersion = "0.7.0",
        .EnterpriseId = BDD_TARGET_ENTERPRISE_ID,
        .GetIpCount = BddTargetIps_Count,
        .GetIpAt = BddTargetIps_At,
    };
    originSd = SolidSyslogOriginSd_Create(&originConfig);
    sdList[0] = metaSd;
    sdList[1] = timeQualitySd;
    sdList[2] = originSd;

    solidSyslogConfig = (struct SolidSyslogConfig) {
        .Buffer = buffer,
        .Sender = switchingSender,
        .Clock = NULL,
        .GetHostname = GetHostname,
        .GetAppName = GetAppName,
        .GetProcessId = NULL,
        .Store = SolidSyslogNullStore_Get(),
        .Sd = sdList,
        .SdCount = sizeof(sdList) / sizeof(sdList[0]),
    };
    SolidSyslog_SetErrorHandler(ErrorHandlerEx, NULL);
    solidSyslog = SolidSyslog_Create(&solidSyslogConfig);
    solidSyslogReady = true;

    BddTargetInteractive_Run(solidSyslog, &testMessage, stdin, BddTargetSwitchConfig_SetByName, OnSet);

    const UBaseType_t interactiveHwm = uxTaskGetStackHighWaterMark(NULL);
    const UBaseType_t serviceHwm = (serviceTaskHandle != NULL) ? uxTaskGetStackHighWaterMark(serviceTaskHandle) : 0U;
    (void) printf(
        "[stack-hwm] interactive=%lu words service=%lu words\n",
        (unsigned long) interactiveHwm,
        (unsigned long) serviceHwm
    );

    TeardownAll();
    vTaskDelete(NULL);
}

static void ServiceTask(void* argument)
{
    (void) argument;
    while ((lifecycleMutex == NULL) || !solidSyslogReady)
    {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    for (;;)
    {
        SolidSyslogMutex_Lock(lifecycleMutex);
        if (solidSyslogTeardown)
        {
            SolidSyslogMutex_Unlock(lifecycleMutex);
            vTaskDelete(NULL);
        }
        if (solidSyslogReady)
        {
            SolidSyslog_Service(solidSyslog);
        }
        SolidSyslogMutex_Unlock(lifecycleMutex);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void TeardownAll(void)
{
    SolidSyslogMutex_Lock(lifecycleMutex);
    solidSyslogTeardown = true;
    solidSyslogReady = false;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = NULL;
    SolidSyslogOriginSd_Destroy(originSd);
    SolidSyslogTimeQualitySd_Destroy(timeQualitySd);
    SolidSyslogMetaSd_Destroy(metaSd);
    SolidSyslogStdAtomicCounter_Destroy(atomicCounter);
    SolidSyslogMutex_Unlock(lifecycleMutex);

    /* Give Service one iteration to observe the teardown flag and self-delete
     * before the lifecycle mutex is destroyed under it. */
    vTaskDelay(pdMS_TO_TICKS(20));
    serviceTaskHandle = NULL;

    SolidSyslogCircularBuffer_Destroy(buffer);
    SolidSyslogFreeRtosMutex_Destroy(bufferMutex);
    SolidSyslogFreeRtosMutex_Destroy(lifecycleMutex);
    lifecycleMutex = NULL;
    SolidSyslogSwitchingSender_Destroy(switchingSender);
    SolidSyslogUdpSender_Destroy(udpSender);
    SolidSyslogStreamSender_Destroy(tcpSender);
    SolidSyslogLwipRawTcpStream_Destroy(tcpStream);
    SolidSyslogLwipRawAddress_Destroy(udpAddress);
    SolidSyslogLwipRawAddress_Destroy(tcpAddress);
    SolidSyslogLwipRawDatagram_Destroy(datagram);
    SolidSyslogLwipRawResolver_Destroy(resolver);
}

static void SemihostingExit(int status)
{
    /* SYS_EXIT_EXTENDED (0x20) — propagates a non-zero status on AArch32 via a
     * { reason, subcode } block. QEMU terminates the VM; the for(;;) is
     * defensive. */
    const struct
    {
        uint32_t reason;
        uint32_t subcode;
    } args = {0x20026U, (uint32_t) status};

    register int r0 __asm("r0") = 0x20;
    register const void* r1 __asm("r1") = &args;
    __asm volatile("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
    for (;;)
    {
    }
}
