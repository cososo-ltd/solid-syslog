/* FreeRTOS-Plus-TCP single-task SolidSyslog example for QEMU mps2-an385.
 *
 * S08.03 wired SolidSyslog over a hardcoded TEST_* configuration and
 * exposed the configurable fields as `set <name> <value>` commands over
 * the interactive UART channel (hostname, appname, procid, msgid, msg,
 * host, port, facility, severity). S08.04 swaps the original NullBuffer
 * for the portable SolidSyslogCircularBuffer + SolidSyslogFreeRtosMutex
 * and adds a dedicated FreeRTOS Service task that drains the ring —
 * Log() is now non-blocking, the Service task does the UDP I/O. S08.05
 * adds the file-backed store: `set store file` tears down NullStore and
 * rebuilds SolidSyslog with FatFsFile + FileBlockDevice + BlockStore on
 * top of QEMU semihosting (see diskio.c). The four `max-blocks`,
 * `max-block-size`, `discard-policy`, `halt-exit` keys update pending
 * globals that the `set store file` trigger picks up.
 *
 * Static IPv4 (10.0.2.15) on the QEMU slirp network with the host
 * reachable at the slirp gateway 10.0.2.2; Bdd/Targets/Common/BddTargetInteractive
 * runs over qemu -serial stdio (CmsdkUart RX wired into newlib's _read
 * in Bdd/Targets/FreeRtos/Common/Syscalls.c). On link-up the IP-task event
 * hook spawns the interactive task and the service task once; UdpSender
 * drives the SolidSyslogFreeRtosDatagram via the static resolver, so
 * each `send N` line over the UART emits N RFC 5424 datagrams to
 * {10.0.2.2, port=port}. */

#include "CmsdkUart.h"
#include "BddTargetEnterpriseId.h"
#include "BddTargetInteractive.h"
#include "BddTargetIps.h"
#include "BddTargetLanguage.h"
#include "BddTargetSwitchConfig.h"
#include "SolidSyslog.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogCrc16Policy.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogError.h"
#include "SolidSyslogFatFsFile.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogFreeRtosDatagram.h"
#include "SolidSyslogFreeRtosMutex.h"
#include "SolidSyslogFreeRtosStaticResolver.h"
#include "SolidSyslogFreeRtosSysUpTime.h"
#include "SolidSyslogFreeRtosTcpStream.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogUdpSender.h"

#include "ff.h" /* f_mount / f_mkfs — FatFs requires an explicit volume mount before any file operation; we eagerly mount-or-format on the `set store file` rebuild trigger. */

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
static const uint8_t TEST_IP_ADDRESS[ipIP_ADDRESS_LENGTH_BYTES] = {10U, 0U, 2U, 15U};
static const uint8_t TEST_NETMASK[ipIP_ADDRESS_LENGTH_BYTES] = {255U, 255U, 255U, 0U};
static const uint8_t TEST_GATEWAY[ipIP_ADDRESS_LENGTH_BYTES] = {10U, 0U, 2U, 2U};
static const uint8_t TEST_DNS[ipIP_ADDRESS_LENGTH_BYTES] = {10U, 0U, 2U, 3U};
static const uint8_t TEST_MAC[ipMAC_ADDRESS_LENGTH_BYTES] = {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U};
static const uint8_t TEST_DESTINATION_IPV4[ipIP_ADDRESS_LENGTH_BYTES] = {10U, 0U, 2U, 2U};

/* Mutable walking-skeleton state. Defaults populated at boot; the
 * interactive `set <name> <value>` command rewrites these in-place via
 * OnSet below. Storage sizes match RFC 5424 maxima where applicable
 * (APP-NAME 48, MSGID 32) plus null terminator; MSG matches
 * SOLIDSYSLOG_MAX_MESSAGE_SIZE so a single `set msg <body>` can carry a
 * full path-MTU-class body; host fits an IPv4 dotted-quad. testMessage
 * holds facility/severity (mutated in place) and the messageId/msg
 * pointers (which target the mutable storage so contents are seen on
 * each Log). */
static char appName[49] = "SolidSyslogBddTarget";
static char messageId[33] = "example";
static char msg[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = "Hello from FreeRTOS";
static char host[16] = "10.0.2.2";
static uint16_t port = (uint16_t) BDD_TARGET_UDP_PORT;
static uint32_t endpointVersion = 0U;

static struct SolidSyslogMessage testMessage = {
    .Facility = SolidSyslogFacility_Local0,
    .Severity = SolidSyslogSeverity_Informational,
    .MessageId = messageId,
    .Msg = msg,
};

/* Plus-TCP requires the network interface descriptor and its endpoint(s)
 * to outlive the IP stack. */
static NetworkInterface_t networkInterface;
static NetworkEndPoint_t networkEndPoint;

static SolidSyslogFreeRtosStaticResolverStorage resolverStorage;
static SolidSyslogFreeRtosDatagramStorage datagramStorage;
static SolidSyslogFreeRtosTcpStreamStorage tcpStreamStorage;
static SolidSyslogStreamSenderStorage tcpSenderStorage;

/* CircularBuffer + FreeRtosMutex composition for cross-task emission.
 * 8 max-sized messages is comfortably above the 3-message BDD scenarios
 * with headroom for a brief Service drain stall, and ~16 KB of .bss is
 * trivial against the mps2-an385's 16 MB SRAM. */
enum
{
    BDD_TARGET_BUFFER_MESSAGES = 8
};

static SolidSyslogCircularBufferStorage
    bufferStorage[SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE(BDD_TARGET_BUFFER_MESSAGES)];
static SolidSyslogFreeRtosMutexStorage mutexStorage;

/* Lifecycle mutex serialises SolidSyslog_Service against the rebuild path
 * (`set store file` swaps NullStore for the file-backed BlockStore by
 * destroying and re-creating SolidSyslog mid-run). Service holds the lock
 * for one Service() call per iteration; the rebuild path holds it across
 * Destroy → BlockStore_Create → Create. */
static SolidSyslogFreeRtosMutexStorage lifecycleMutexStorage;
static struct SolidSyslogMutex* lifecycleMutex = NULL;
static volatile bool solidSyslogReady;
/* Signals Service to self-delete BEFORE Teardown destroys the lifecycle
 * mutex. Without this, Service races against InteractiveTask: Teardown
 * destroys lifecycleMutex and NULLs it, but Service's next iteration
 * unconditionally locks lifecycleMutex — NULL deref or use-after-free.
 * Set inside the lifecycle-mutex critical section so Service observes it
 * atomically with the SolidSyslog destroy. */
static volatile bool solidSyslogTeardown = false;

/* File-backed store storage. Lives in .bss so it persists across the
 * `set store file` rebuild; only populated when that command fires.
 * STORE_PATH_PREFIX is "STORE" — sequence-numbered FAT filenames land as
 * STORE00.log, STORE01.log, … which fit 8.3 short-filename mode (LFN=0
 * in our ffconf.h). */
static const char STORE_PATH_PREFIX[] = "STORE";

static SolidSyslogFatFsFileStorage storeFileStorage;
static SolidSyslogFileBlockDeviceStorage blockDeviceStorage;
static SolidSyslogBlockStoreStorage blockStoreStorage;

/* FATFS object lives in .bss because f_mount stores its address inside the
 * FatFs volume registry — the object must outlive every f_open / f_stat /
 * f_unlink. One per volume (FF_VOLUMES = 1). */
static FATFS fatfs;
static bool fatfsMounted = false;

static struct SolidSyslogFile* storeFile = NULL;
static struct SolidSyslogBlockDevice* storeBlockDevice = NULL;
static struct SolidSyslogStore* currentStore = NULL;
static bool currentStoreIsFile = false;

/* Pending values populated by the four `set max-blocks` / `max-block-size`
 * / `discard-policy` / `halt-exit` commands and consumed by `set store
 * file`. Defaults mirror Linux's BddTargetCommandLine (DEFAULT_MAX_BLOCKS=10,
 * DEFAULT_MAX_BLOCK_SIZE=65536, discardPolicy="oldest", halt-exit=false). */
enum
{
    DEFAULT_PENDING_MAX_BLOCKS = 10,
    DEFAULT_PENDING_MAX_BLOCK_SIZE = 65536,
};

static size_t pendingMaxBlocks = DEFAULT_PENDING_MAX_BLOCKS;
static size_t pendingMaxBlockSize = DEFAULT_PENDING_MAX_BLOCK_SIZE;
static const char* pendingDiscardPolicy = "oldest";
static volatile bool pendingHaltExit = false;
static size_t pendingCapacityThreshold = 0;
/* When true, SolidSyslog gets only the meta SD (sequenceId / sysUpTime /
 * language) — timeQuality and origin are dropped. Mirrors Linux's
 * --no-sd. Consumed by the initial Setup and by RebuildWithFileStore. */
static volatile bool pendingNoSd = false;

/* Holds the final SolidSyslog config so the rebuild path can rewrite
 * .Store and pass the same struct back into SolidSyslog_Create. */
static struct SolidSyslogConfig solidSyslogConfig;
static struct SolidSyslogStructuredData* sdList[3];
static struct SolidSyslogAtomicCounter* atomicCounter = NULL;
static struct SolidSyslogStructuredData* metaSd = NULL;
static struct SolidSyslogStructuredData* timeQualitySd = NULL;
static struct SolidSyslogStructuredData* originSd = NULL;

/* Resources allocated in InteractiveTask's Setup phase and released by
 * TeardownAll. File-scope static so the two exit paths can reach the
 * destroy chain: the interactive `quit` command (which returns from
 * BddTargetInteractive_Run and falls through to TeardownAll in the same
 * task), and `set shutdown 1` from the OnSet handler (which calls
 * TeardownAll then SemihostingExit). */
static struct SolidSyslogResolver* resolver = NULL;
static struct SolidSyslogDatagram* datagram = NULL;
static struct SolidSyslogStream* tcpStream = NULL;
static struct SolidSyslogSender* tcpSender = NULL;
static struct SolidSyslogBuffer* buffer = NULL;
static struct SolidSyslogMutex* bufferMutex = NULL;

/* Ensures the interactive task is created exactly once even if the network
 * goes down and back up. */
static BaseType_t interactiveTaskCreated = pdFALSE;

/* Service task handle is captured at creation so the interactive task can
 * report its peak stack usage alongside its own on `quit`. */
static TaskHandle_t serviceTaskHandle = NULL;

extern NetworkInterface_t* pxMPS2_FillInterfaceDescriptor(BaseType_t xEMACIndex, NetworkInterface_t* pxInterface);

static bool TryUpdateString(char* storage, size_t storageSize, const char* value);
static bool TryParseUInt(const char* value, unsigned long* out);
static bool RebuildWithFileStore(void);
static void DestroyCurrentStore(void);
static void TeardownAll(void);
static bool EnsureFatFsMounted(void);
static enum SolidSyslogDiscardPolicy MapDiscardPolicy(const char* policy);
static void OnStoreFull(void* context);
static size_t GetCapacityThreshold(void* context);
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
    *ipr = ETHERNET_IRQ_PRIORITY;
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
    char ipBuffer[16];
    FreeRTOS_GetEndPointConfiguration(&ipAddress, NULL, NULL, NULL, &networkEndPoint);
    FreeRTOS_inet_ntoa(ipAddress, ipBuffer);
    SolidSyslogFormatter_BoundedString(formatter, ipBuffer, strlen(ipBuffer));
}

static void GetAppName(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_BoundedString(formatter, appName, strlen(appName));
}

/* No RTC and no time-sync on this reference target — the example models an
 * embedded device that has no concept of wall-clock time. RFC 5424 §6.2.3.1
 * mandates NILVALUE TIMESTAMP in that case, and the timeQuality SD reports
 * tzKnown=0, isSynced=0. SolidSyslogConfig.Clock=NULL drops through to the
 * library's NilClock; the resulting all-zero SolidSyslogTimestamp fails
 * TimestampIsValid in Core/Source/SolidSyslog.c and emits "-" on the wire. */
static void ErrorHandler(void* context, enum SolidSyslogSeverity severity, const char* message)
{
    (void) context;
    (void) printf("[solidsyslog] severity=%d %s\n", (int) severity, message);
}

static void GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->TzKnown = false;
    timeQuality->IsSynced = false;
    timeQuality->SyncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
}

static void GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    /* SolidSyslogFreeRtosStaticResolver currently ignores the host
     * string and routes via TEST_DESTINATION_IPV4, so host is plumbed
     * here for forward-compatibility with the follow-up slice that will
     * teach the resolver to parse dotted-quads. The port reaches the
     * wire via sendto unchanged. */
    SolidSyslogFormatter_BoundedString(endpoint->Host, host, strlen(host));
    endpoint->Port = port;
}

static uint32_t GetEndpointVersion(void)
{
    return endpointVersion;
}

static bool OnSet(const char* name, const char* value)
{
    if (strcmp(name, "appname") == 0)
    {
        return TryUpdateString(appName, sizeof(appName), value);
    }
    if (strcmp(name, "msgid") == 0)
    {
        return TryUpdateString(messageId, sizeof(messageId), value);
    }
    if (strcmp(name, "msg") == 0)
    {
        return TryUpdateString(msg, sizeof(msg), value);
    }
    if (strcmp(name, "host") == 0)
    {
        return TryUpdateString(host, sizeof(host), value);
    }
    if (strcmp(name, "port") == 0)
    {
        unsigned long parsed = 0U;
        if (!TryParseUInt(value, &parsed) || parsed == 0U || parsed > UINT16_MAX)
        {
            return false;
        }
        port = (uint16_t) parsed;
        endpointVersion++;
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
        testMessage.Facility = (enum SolidSyslogFacility) parsed;
        return true;
    }
    if (strcmp(name, "severity") == 0)
    {
        unsigned long parsed = 0U;
        if (!TryParseUInt(value, &parsed))
        {
            return false;
        }
        testMessage.Severity = (enum SolidSyslogSeverity) parsed;
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
    if (strcmp(name, "max-blocks") == 0)
    {
        unsigned long parsed = 0U;
        if (!TryParseUInt(value, &parsed))
        {
            return false;
        }
        pendingMaxBlocks = (size_t) parsed;
        return true;
    }
    if (strcmp(name, "max-block-size") == 0)
    {
        unsigned long parsed = 0U;
        if (!TryParseUInt(value, &parsed))
        {
            return false;
        }
        pendingMaxBlockSize = (size_t) parsed;
        return true;
    }
    if (strcmp(name, "discard-policy") == 0)
    {
        if ((strcmp(value, "oldest") != 0) && (strcmp(value, "newest") != 0) && (strcmp(value, "halt") != 0))
        {
            return false;
        }
        /* String literal storage — target_driver.py emits one of the three
         * literals above so the pointer stays valid (no copy needed). */
        pendingDiscardPolicy =
            (strcmp(value, "newest") == 0) ? "newest" : ((strcmp(value, "halt") == 0) ? "halt" : "oldest");
        return true;
    }
    if (strcmp(name, "halt-exit") == 0)
    {
        /* Harness emits `set halt-exit 1` (or `0`) because the FreeRTOS
         * `set` protocol always carries a value; on Linux the equivalent
         * is a bare --halt-exit flag (no_argument). Anything non-zero
         * trips the halt path. */
        unsigned long parsed = 0U;
        if (!TryParseUInt(value, &parsed))
        {
            return false;
        }
        pendingHaltExit = (parsed != 0U);
        return true;
    }
    if (strcmp(name, "no-sd") == 0)
    {
        /* `set no-sd 1` drops the SD list to only metaSd — mirrors
         * Linux's --no-sd bare flag. The setting takes effect via
         * SolidSyslog re-Create (initial Setup or `set store file`
         * rebuild). */
        unsigned long parsed = 0U;
        if (!TryParseUInt(value, &parsed))
        {
            return false;
        }
        pendingNoSd = (parsed != 0U);
        return true;
    }
    if (strcmp(name, "store") == 0)
    {
        /* "null" is the default state — accept it as an unconditional
         * no-op so the harness can pass --store null without us needing
         * to special-case it on the harness side. "file" triggers the
         * rebuild (which is one-way for the lifetime of this QEMU
         * instance — see RebuildWithFileStore). */
        if (strcmp(value, "null") == 0)
        {
            return true;
        }
        if (strcmp(value, "file") == 0)
        {
            return RebuildWithFileStore();
        }
        return false;
    }
    if (strcmp(name, "shutdown") == 0)
    {
        /* Any non-zero value triggers a full teardown then exits QEMU.
         * Same destroy chain as the `quit` path; only the final action
         * differs (SemihostingExit here, vTaskDelete on `quit`). The BDD
         * `the client is killed` step on freertos sends `set shutdown 1`. */
        unsigned long parsed = 0U;
        if (!TryParseUInt(value, &parsed))
        {
            return false;
        }
        if (parsed != 0U)
        {
            TeardownAll();
            SemihostingExit(0);
        }
        return true;
    }
    return false;
}

static enum SolidSyslogDiscardPolicy MapDiscardPolicy(const char* policy)
{
    if (strcmp(policy, "newest") == 0)
    {
        return SolidSyslogDiscardPolicy_Newest;
    }
    if (strcmp(policy, "halt") == 0)
    {
        return SolidSyslogDiscardPolicy_Halt;
    }
    return SolidSyslogDiscardPolicy_Oldest;
}

static void OnStoreFull(void* context)
{
    (void) context;
    if (pendingHaltExit)
    {
        /* Semihosting SYS_EXIT — terminates QEMU with the given status so
         * the BDD harness sees the run end deterministically. Mirrors the
         * Linux example's _exit(2) (Bdd/Targets/Linux/main.c::OnStoreFull). */
        SemihostingExit(2);
    }
}

static size_t GetCapacityThreshold(void* context)
{
    return *(const size_t*) context;
}

static bool RebuildWithFileStore(void)
{
    /* Lifecycle mutex blocks the Service task from running SolidSyslog_Service
     * across the Destroy → re-Create transition. Service waits on the next
     * iteration's lock; rebuild releases when done. */
    SolidSyslogMutex_Lock(lifecycleMutex);

    /* FatFs does NOT auto-mount on first f_open — the integrator must call
     * f_mount before any file operation, and f_mkfs when the volume is
     * fresh. EnsureFatFsMounted handles both. Doing this BEFORE tearing down
     * the existing store means a mount failure leaves the target running on
     * the original NullStore (zero-disruption); we return false so OnSet
     * reports the failure to the harness. */
    if (!EnsureFatFsMounted())
    {
        SolidSyslogMutex_Unlock(lifecycleMutex);
        return false;
    }

    solidSyslogReady = false;
    SolidSyslog_Destroy();
    DestroyCurrentStore();

    /* Build a fresh FatFs-backed BlockStore. With the volume mounted above,
     * BlockSequence_Open's f_stat / f_open calls now hit a live filesystem
     * via disk_read / disk_write semihosting traps. */
    storeFile = SolidSyslogFatFsFile_Create(&storeFileStorage);
    storeBlockDevice = SolidSyslogFileBlockDevice_Create(&blockDeviceStorage, storeFile, STORE_PATH_PREFIX);

    struct SolidSyslogSecurityPolicy* policy = SolidSyslogCrc16Policy_Create();
    struct SolidSyslogBlockStoreConfig storeConfig = {
        .BlockDevice = storeBlockDevice,
        .MaxBlockSize = pendingMaxBlockSize,
        .MaxBlocks = pendingMaxBlocks,
        .DiscardPolicy = MapDiscardPolicy(pendingDiscardPolicy),
        .SecurityPolicy = policy,
        .OnStoreFull = OnStoreFull,
        .StoreFullContext = NULL,
        .GetCapacityThreshold = GetCapacityThreshold,
        .OnThresholdCrossed = NULL,
        .ThresholdContext = &pendingCapacityThreshold,
    };
    currentStore = SolidSyslogBlockStore_Create(&blockStoreStorage, &storeConfig);
    currentStoreIsFile = true;

    solidSyslogConfig.Store = currentStore;
    /* Re-honour `set no-sd 1` if it arrived before this rebuild — the
     * sort order in target_driver.py guarantees `set no-sd` comes before
     * `set store file` so the value is final by the time we get here. */
    solidSyslogConfig.SdCount = pendingNoSd ? 1U : (sizeof(sdList) / sizeof(sdList[0]));
    SolidSyslog_Create(&solidSyslogConfig);
    solidSyslogReady = true;
    SolidSyslogMutex_Unlock(lifecycleMutex);
    return true;
}

/* Tears down whichever store is currently installed (file-backed or null).
 * Shared by RebuildWithFileStore (which then re-creates) and
 * ShutdownGracefully (which then exits). FatFsFile_Destroy → Close →
 * f_close flushes the underlying FIL's dir entry. */
static void DestroyCurrentStore(void)
{
    if (currentStoreIsFile)
    {
        SolidSyslogBlockStore_Destroy(currentStore);
        SolidSyslogFileBlockDevice_Destroy(storeBlockDevice);
        SolidSyslogCrc16Policy_Destroy();
        SolidSyslogFatFsFile_Destroy(storeFile);
    }
    else
    {
        SolidSyslogNullStore_Destroy();
    }
}

/* Full teardown of every resource InteractiveTask allocated during Setup.
 * Two entry points — `quit` (falls through after BddTargetInteractive_Run
 * returns) and `set shutdown 1` (OnSet handler) — both route through here
 * so the destroy chain is single-source-of-truth, not duplicated. f_unmount
 * fires regardless of how we got here; the BDD power_cycle_replay scenario
 * relies on this so the next session's f_mount finds STORE*.log directory
 * entries up-to-date. The lifecycle mutex held across the SolidSyslog +
 * store destroy keeps Service from racing the teardown. */
static void TeardownAll(void)
{
    SolidSyslogMutex_Lock(lifecycleMutex);
    solidSyslogTeardown = true;
    solidSyslogReady = false;
    SolidSyslog_Destroy();
    SolidSyslogOriginSd_Destroy();
    SolidSyslogTimeQualitySd_Destroy();
    SolidSyslogMetaSd_Destroy();
    SolidSyslogAtomicCounter_Destroy();
    DestroyCurrentStore();
    if (fatfsMounted)
    {
        (void) f_unmount("");
        fatfsMounted = false;
    }
    SolidSyslogMutex_Unlock(lifecycleMutex);

    /* Give Service one full iteration (vTaskDelay 1ms + lock-check) to
     * observe the teardown flag and vTaskDelete itself before we destroy
     * the lifecycle mutex out from under it. 20ms is generous against
     * Service's worst-case iteration time. */
    vTaskDelay(pdMS_TO_TICKS(20));
    serviceTaskHandle = NULL;

    SolidSyslogCircularBuffer_Destroy(buffer);
    SolidSyslogFreeRtosMutex_Destroy(bufferMutex);
    SolidSyslogFreeRtosMutex_Destroy(lifecycleMutex);
    lifecycleMutex = NULL;
    SolidSyslogSwitchingSender_Destroy();
    SolidSyslogStreamSender_Destroy(tcpSender);
    SolidSyslogFreeRtosTcpStream_Destroy(tcpStream);
    SolidSyslogUdpSender_Destroy();
    SolidSyslogFreeRtosDatagram_Destroy(datagram);
    SolidSyslogFreeRtosStaticResolver_Destroy(resolver);
}

/* Mount volume 0; format-on-first-use if the disk image has no FAT yet.
 * Idempotent — subsequent calls short-circuit on fatfsMounted. The work
 * buffer for f_mkfs is sized to FF_MAX_SS (512 B) which is the minimum
 * f_mkfs accepts on a FAT12/16 volume. */
static bool EnsureFatFsMounted(void)
{
    if (fatfsMounted)
    {
        return true;
    }
    FRESULT res = f_mount(
        &fatfs,
        "",
        1
    ); /* opt=1 → mount immediately, surface FR_NO_FILESYSTEM here rather than at first f_open */
    if (res == FR_NO_FILESYSTEM)
    {
        /* Fresh disk image — lay down a FAT and re-mount. FAT12 is the
         * natural choice for a 1 MiB volume (small enough that FAT32's
         * cluster overhead would dominate). */
        static BYTE workBuffer[FF_MAX_SS];
        const MKFS_PARM opts = {.fmt = FM_FAT | FM_SFD, .n_fat = 1, .align = 1, .n_root = 0, .au_size = 0};
        res = f_mkfs("", &opts, workBuffer, sizeof(workBuffer));
        if (res == FR_OK)
        {
            res = f_mount(&fatfs, "", 1);
        }
    }
    if (res != FR_OK)
    {
        (void) printf("[solidsyslog] fatfs mount failed: FRESULT=%d\n", (int) res);
        return false;
    }
    fatfsMounted = true;
    return true;
}

static void SemihostingExit(int status)
{
    /* SYS_EXIT_EXTENDED (0x20) — the only ARM Semihosting exit form on
     * AArch32 that propagates a non-zero status: R1 points to a
     * { reason, subcode } parameter block. The simpler SYS_EXIT (0x18)
     * on AArch32 takes R1 as a *literal* reason code (ADP_Stopped_*),
     * so passing a struct pointer there resolved to "unrecognised reason"
     * → QEMU exit 1 regardless of subcode. Marked unreachable after the
     * trap because QEMU terminates the VM; the for(;;) is defensive. */
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
    char* end = NULL;
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

    resolver = SolidSyslogFreeRtosStaticResolver_Create(&resolverStorage, TEST_DESTINATION_IPV4);
    datagram = SolidSyslogFreeRtosDatagram_Create(&datagramStorage);

    struct SolidSyslogUdpSenderConfig udpConfig = {
        .Resolver = resolver,
        .Datagram = datagram,
        .Endpoint = GetEndpoint,
        .EndpointVersion = GetEndpointVersion,
    };
    struct SolidSyslogSender* udpSender = SolidSyslogUdpSender_Create(&udpConfig);

    /* Plain TCP path via the new FreeRTOS Plus-TCP stream adapter. Shares the
     * UDP endpoint callbacks because the BDD oracle (syslog-ng) listens on the
     * same host:port for both transports — the syslog-ng config in
     * Bdd/syslog-ng/syslog-ng.conf has a TCP listener on 5514 alongside UDP. */
    tcpStream = SolidSyslogFreeRtosTcpStream_Create(&tcpStreamStorage);
    struct SolidSyslogStreamSenderConfig tcpConfig = {
        .Resolver = resolver,
        .Stream = tcpStream,
        .Endpoint = GetEndpoint,
        .EndpointVersion = GetEndpointVersion,
    };
    tcpSender = SolidSyslogStreamSender_Create(&tcpSenderStorage, &tcpConfig);

    /* SwitchingSender lets `set transport <udp|tcp>` flip the active transport
     * at runtime. Default to UDP so existing UDP-tagged scenarios stay green;
     * `--transport tcp` flowing through the behave harness lands here as
     * `set transport tcp` over the UART and switches before the first send. */
    static struct SolidSyslogSender* inners[2];
    inners[BDD_TARGET_SWITCH_UDP] = udpSender;
    inners[BDD_TARGET_SWITCH_TCP] = tcpSender;
    struct SolidSyslogSwitchingSenderConfig switchConfig = {
        .Senders = inners,
        .SenderCount = sizeof(inners) / sizeof(inners[0]),
        .Selector = BddTargetSwitchConfig_Selector,
    };
    BddTargetSwitchConfig_SetByName("udp");
    struct SolidSyslogSender* sender = SolidSyslogSwitchingSender_Create(&switchConfig);

    /* CircularBuffer drained by ServiceTask below, with a FreeRtosMutex
     * gating concurrent producers (interactive task today; multi-task
     * emission in S08.04 slice 3 will add more). The buffer's Read side
     * is the Service task; its Write side is whichever task calls
     * SolidSyslog_Log. */
    bufferMutex = SolidSyslogFreeRtosMutex_Create(&mutexStorage);
    buffer = SolidSyslogCircularBuffer_Create(bufferStorage, sizeof(bufferStorage), bufferMutex);

    /* Lifecycle mutex created up front so the Service task can take it
     * from its very first iteration without a NULL check. */
    lifecycleMutex = SolidSyslogFreeRtosMutex_Create(&lifecycleMutexStorage);

    /* Default store is NullStore — flipped to FatFs/BlockStore by
     * `set store file` via RebuildWithFileStore(). */
    currentStore = SolidSyslogNullStore_Create();
    currentStoreIsFile = false;

    atomicCounter = SolidSyslogAtomicCounter_Create();
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
        .Sender = sender,
        .Clock = NULL,
        .GetHostname = GetHostname,
        .GetAppName = GetAppName,
        /* PROCID — RFC 5424 §6.2.6 NILVALUE: FreeRTOS QEMU has no
         * process model. NULL drops through to the library's
         * NilStringFunction which yields an empty field; FormatStringField
         * (Core/Source/SolidSyslog.c) then emits "-" on the wire. */
        .GetProcessId = NULL,
        .Store = currentStore,
        .Sd = sdList,
        /* pendingNoSd is normally false at this initial Setup call —
         * the `set no-sd 1` translation runs over the UART AFTER the
         * prompt is up. Slice 6's @store scenarios on FreeRTOS always
         * couple --no-sd with --store file, so the rebuild path rewrites
         * .SdCount with the up-to-date value. This initial value is
         * defensive in case a future scenario sends `set no-sd 1` before
         * any rebuild. */
        .SdCount = pendingNoSd ? 1U : (sizeof(sdList) / sizeof(sdList[0])),
    };
    SolidSyslog_SetErrorHandler(ErrorHandler, NULL);
    SolidSyslog_Create(&solidSyslogConfig);
    solidSyslogReady = true;

    BddTargetInteractive_Run(&testMessage, stdin, BddTargetSwitchConfig_SetByName, OnSet);

    /* Peak stack usage report on `quit`. Captured into every BDD run's QEMU
     * console output so stack regressions surface in bdd-freertos-qemu logs
     * and so E21 tunable changes (S21.02 onward) leave an empirical trail
     * for the deferred stack-shrink optimisation to consume. Words, not
     * bytes — FreeRTOS reports min free stack in StackType_t units (4 B on
     * Cortex-M3). serviceTaskHandle is guarded because
     * uxTaskGetStackHighWaterMark(NULL) means "the calling task" — passing a
     * NULL handle after a Service xTaskCreate failure would silently
     * double-report the interactive task's HWM. */
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

/* Drains the CircularBuffer in the background while the interactive task
 * (and, in S08.04 slice 3, additional worker tasks) call SolidSyslog_Log.
 * Equal priority to the producers — FreeRTOS round-robins time slices
 * between same-priority ready tasks so the buffer is drained without the
 * Service task starving slower producers. */
#define SERVICE_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 16U)

static void ServiceTask(void* argument)
{
    (void) argument;
    /* Wait until the interactive task has finished initial Setup and
     * created the lifecycle mutex / SolidSyslog. After that, the mutex is
     * the source of truth — Setup, RebuildWithFileStore, and Teardown all
     * hold it across their Destroy/Create transitions, so the ready flag
     * is only checked after we win the lock. */
    while ((lifecycleMutex == NULL) || !solidSyslogReady)
    {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    for (;;)
    {
        SolidSyslogMutex_Lock(lifecycleMutex);
        if (solidSyslogTeardown)
        {
            /* Teardown set this flag inside the lifecycle critical section,
             * then is sleeping briefly waiting for us to exit. Release and
             * self-delete before Teardown destroys the mutex. */
            SolidSyslogMutex_Unlock(lifecycleMutex);
            vTaskDelete(NULL);
        }
        if (solidSyslogReady)
        {
            SolidSyslog_Service();
        }
        SolidSyslogMutex_Unlock(lifecycleMutex);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void vApplicationIPNetworkEventHook_Multi(eIPCallbackEvent_t eNetworkEvent, struct xNetworkEndPoint* pxEndPoint)
{
    (void) pxEndPoint;
    if ((eNetworkEvent == eNetworkUp) && (interactiveTaskCreated == pdFALSE))
    {
        if (xTaskCreate(
                InteractiveTask,
                "interactive",
                INTERACTIVE_TASK_STACK_DEPTH,
                NULL,
                tskIDLE_PRIORITY + 1,
                NULL
            ) == pdPASS)
        {
            (void) xTaskCreate(
                ServiceTask,
                "service",
                SERVICE_TASK_STACK_DEPTH,
                NULL,
                tskIDLE_PRIORITY + 1,
                &serviceTaskHandle
            );
            interactiveTaskCreated = pdTRUE;
        }
    }
}

int main(void)
{
    CmsdkUart_Init(&MMIO_ACCESS, CMSDK_UART0_BASE_ADDRESS);

    SetEthernetIrqPriority();

    (void) pxMPS2_FillInterfaceDescriptor(0, &networkInterface);
    FreeRTOS_FillEndPoint(
        &networkInterface,
        &networkEndPoint,
        TEST_IP_ADDRESS,
        TEST_NETMASK,
        TEST_GATEWAY,
        TEST_DNS,
        TEST_MAC
    );

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

uint32_t ulApplicationGetNextSequenceNumber(
    uint32_t ulSourceAddress,
    uint16_t usSourcePort,
    uint32_t ulDestinationAddress,
    uint16_t usDestinationPort
)
{
    (void) ulSourceAddress;
    (void) usSourcePort;
    (void) ulDestinationAddress;
    (void) usDestinationPort;
    return (uint32_t) xTaskGetTickCount();
}
