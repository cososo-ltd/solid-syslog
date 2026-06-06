/* FreeRTOS + lwIP (Raw API, NO_SYS=0) SolidSyslog BDD target for QEMU
 * mps2-an385.
 *
 * The platform-independent pipeline — SolidSyslog lifecycle, FatFs-backed store
 * + security policies, SD set, the interactive `set` handler, the Service drain
 * task, and the console glue — lives in Bdd/Targets/Common/BddTargetFreeRtosPipeline
 * (shared with the FreeRTOS-Plus-TCP target, S29.03). This file keeps only the
 * lwIP network backend behind the pipeline seam: the tcpip thread + tcpip_callback
 * marshal (S28.06), the hand-written LAN9118 netif (netif/EthernetIf.c), the
 * static-IP bring-up + gateway ARP warm-up, the LwipRaw sender wiring (UDP +
 * octet-framed TCP + TLS/mTLS via mbedTLS over a second LwipRaw TCP), and the
 * RFC 5424 HOSTNAME read from the netif.
 *
 * Static IPv4 (10.0.2.15) on the QEMU slirp network, host reachable at the slirp
 * gateway 10.0.2.2. The oracle is addressed by name ("syslog-ng") via
 * SolidSyslogLwipRawDnsResolver; lwIP's DNS_LOCAL_HOSTLIST (see lwipopts.h) maps
 * that name statically to 10.0.2.2 (slirp can't return a reachable address for
 * the docker alias over real DNS). */

#include "BddTargetFatFsMount.h"
#include "BddTargetFreeRtosPipeline.h"
#include "EthernetIf.h"

#include "BddTargetSwitchConfig.h"
#include "BddTargetTlsSender.h"

#include "SolidSyslogHeaderField.h"
#include "SolidSyslogLwipRawAddress.h"
#include "SolidSyslogLwipRawDatagram.h"
#include "SolidSyslogLwipRawDnsResolver.h"
#include "SolidSyslogLwipRawMarshal.h"
#include "SolidSyslogLwipRawTcpStream.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogSwitchingSender.h"
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
#include <string.h>

#define CMSDK_UART0_BASE_ADDRESS UINT32_C(0x40004000)

/* lwIP netif descriptor — must outlive the tcpip thread. */
static struct netif networkInterface;
/* Gateway IP, kept at file scope so the ARP warm-up can reach it after bring-up. */
static ip4_addr_t gatewayAddress;

/* LwipRaw sender adapters — built by BuildSender on the interactive task, torn
 * down by TeardownNetwork. */
static struct SolidSyslogResolver* resolver = NULL;
static struct SolidSyslogDatagram* datagram = NULL;
static struct SolidSyslogAddress* udpAddress = NULL;
static struct SolidSyslogSender* udpSender = NULL;
static struct SolidSyslogStream* tcpStream = NULL;
static struct SolidSyslogAddress* tcpAddress = NULL;
static struct SolidSyslogSender* tcpSender = NULL;
static struct SolidSyslogSender* switchingSender = NULL;

static void LwipTcpipMarshal(SolidSyslogLwipRawCallback callback, void* context);
static void NetworkBringUp(void* context);
static void WarmUpGatewayArp(void);
static void GatewayResolvedQuery(void* context);
static void GetHostname(struct SolidSyslogHeaderField* field, void* context);
static struct SolidSyslogSender* BuildSender(void);
static void TeardownNetwork(void);

static const struct BddTargetFreeRtosPipelineConfig PIPELINE_CONFIG = {
    .DefaultHost = "syslog-ng",
    .BuildSender = BuildSender,
    .GetHostname = GetHostname,
    .TeardownNetwork = TeardownNetwork,
    .MountStore = BddTargetFatFsMount_Mount,
    .UnmountStore = BddTargetFatFsMount_Unmount,
    .CreateStoreFile = BddTargetFatFsMount_CreateFile,
    .DestroyStoreFile = BddTargetFatFsMount_DestroyFile,
};

/* lwIP randomness source (declared by arch/cc.h's LWIP_RAND). sys_now() comes
 * from the contrib FreeRTOS sys_arch under NO_SYS=0. A self-contained xorshift32
 * keeps TCP ISN selection deterministic without a real entropy backend;
 * adequate for the BDD smoke test. */
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
    BddTargetFreeRtosPipeline_InitConsole(CMSDK_UART0_BASE_ADDRESS);
    BddTargetFreeRtosPipeline_SetConfig(&PIPELINE_CONFIG);

    /* Pin every LwipRaw adapter call onto the tcpip thread. */
    SolidSyslogLwipRaw_SetMarshal(LwipTcpipMarshal);

    /* Create the tcpip thread + mbox + core-lock mutex. Pre-scheduler safe; the
     * thread runs once the scheduler starts. The netif bring-up is deferred to
     * NetworkBringUp on that thread (smsc9220_init calls vTaskDelay, which would
     * deref a NULL pxCurrentTCB before the scheduler). */
    tcpip_init(NULL, NULL);

    if (xTaskCreate(
            BddTargetFreeRtosPipeline_InteractiveTask,
            "interactive",
            configMINIMAL_STACK_SIZE * BDD_TARGET_INTERACTIVE_STACK_MULTIPLIER,
            NULL,
            tskIDLE_PRIORITY + 1,
            NULL
        ) != pdPASS)
    {
        BddTargetFreeRtosPipeline_Exit(1);
    }
    if (xTaskCreate(
            BddTargetFreeRtosPipeline_ServiceTask,
            "service",
            configMINIMAL_STACK_SIZE * BDD_TARGET_SERVICE_STACK_MULTIPLIER,
            NULL,
            tskIDLE_PRIORITY + 1,
            NULL
        ) != pdPASS)
    {
        BddTargetFreeRtosPipeline_Exit(1);
    }

    vTaskStartScheduler();

    for (;;)
    {
    }
    return 0;
}

/* Runs on the tcpip thread (dispatched via tcpip_callback once the scheduler is
 * up) so netif_add, the link/up transitions, and smsc9220_init's vTaskDelay all
 * execute in a valid task context with the lwIP core lock held. */
static void NetworkBringUp(void* context)
{
    (void) context;
    ip4_addr_t ipAddress;
    ip4_addr_t netmask;
    /* QEMU slirp default: 10.0.2.15 guest, 10.0.2.2 gateway (NATed to the QEMU
     * host, where the syslog-ng oracle listens). */
    IP4_ADDR(&ipAddress, 10, 0, 2, 15);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gatewayAddress, 10, 0, 2, 2);

    (void) netif_add(&networkInterface, &ipAddress, &netmask, &gatewayAddress, NULL, EthernetIf_Init, tcpip_input);
    netif_set_default(&networkInterface);
    netif_set_up(&networkInterface);
    netif_set_link_up(&networkInterface);

    /* Kick off ARP resolution for the gateway now, so the cache is warm before
     * the first datagram (SolidSyslogLwipRawDatagram sends PBUF_REF packets; an
     * ARP miss on the first send would drop it). The reply is processed by this
     * tcpip thread; the interactive task waits via WarmUpGatewayArp. */
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

/* Blocks the calling (interactive) task — never the tcpip thread — until the
 * gateway resolves or a bounded deadline passes. Generous deadline: QEMU is
 * markedly slower than host. */
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
         * the core lock and writes `resolved` before this returns. */
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
     * blocks until the work is queued, so it cannot satisfy that alone.
     * LWIP_TCPIP_CORE_LOCKING is enabled, so we run the callback in the caller's
     * own task context under the core lock instead: unconditionally synchronous,
     * independent of task priority, no per-send mailbox message. The lock is
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

static void GetHostname(struct SolidSyslogHeaderField* field, void* context)
{
    /* RFC 5424 §6.2.4 rung 2 (static IP) — read back from the netif so a future
     * DHCP slice satisfies the same rung without touching this callback. */
    const char* address = ip4addr_ntoa(netif_ip4_addr(&networkInterface));
    (void) context;
    SolidSyslogHeaderField_PrintUsAscii(field, address, strlen(address));
}

/* Bring up the netif on the tcpip thread, warm the gateway ARP, then build the
 * LwipRaw SwitchingSender: UDP, octet-framed TCP, and a TLS/mTLS slot (mbedTLS
 * over a second LwipRaw TCP stream). Default transport UDP. Runs on the
 * interactive task — LwipRaw adapters touch a started lwIP core, which is now up. */
static struct SolidSyslogSender* BuildSender(void)
{
    /* Bring the netif up on the tcpip thread now the scheduler is running, then
     * wait for the gateway ARP so the first datagram is not lost to a cache miss. */
    (void) tcpip_callback(NetworkBringUp, NULL);
    WarmUpGatewayArp();

    /* SolidSyslogLwipRawDnsResolver resolves the oracle by name; the
     * DNS_LOCAL_HOSTLIST entry maps "syslog-ng" -> 10.0.2.2 on the guest, so the
     * destination host equals the TLS serverName / cert subject without any
     * numeric pin. The shared Sleep drives the bounded async-resolve spin (a
     * local-hostlist hit returns synchronously, so it never actually spins). */
    struct SolidSyslogLwipRawDnsResolverConfig dnsConfig = {
        .Sleep = BddTargetFreeRtosPipeline_Sleep,
    };
    resolver = SolidSyslogLwipRawDnsResolver_Create(&dnsConfig);
    datagram = SolidSyslogLwipRawDatagram_Create();
    udpAddress = SolidSyslogLwipRawAddress_Create();
    struct SolidSyslogUdpSenderConfig udpConfig = {
        .Resolver = resolver,
        .Datagram = datagram,
        .Address = udpAddress,
        .Endpoint = BddTargetFreeRtosPipeline_GetEndpoint,
        .EndpointVersion = BddTargetFreeRtosPipeline_GetEndpointVersion,
    };
    udpSender = SolidSyslogUdpSender_Create(&udpConfig);

    /* Plain TCP: RFC 6587 octet-framed StreamSender over the LwipRaw TCP stream.
     * The connect timeout comes from the SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS
     * tunable (GetConnectTimeoutMs NULL); the shared Sleep drives the bounded
     * synchronous-connect spin. */
    struct SolidSyslogLwipRawTcpStreamConfig tcpStreamConfig = {
        .GetConnectTimeoutMs = NULL,
        .ConnectTimeoutContext = NULL,
        .Sleep = BddTargetFreeRtosPipeline_Sleep,
    };
    tcpStream = SolidSyslogLwipRawTcpStream_Create(&tcpStreamConfig);
    tcpAddress = SolidSyslogLwipRawAddress_Create();
    struct SolidSyslogStreamSenderConfig tcpConfig = {
        .Resolver = resolver,
        .Stream = tcpStream,
        .Address = tcpAddress,
        .Endpoint = BddTargetFreeRtosPipeline_GetEndpoint,
        .EndpointVersion = BddTargetFreeRtosPipeline_GetEndpointVersion,
    };
    tcpSender = SolidSyslogStreamSender_Create(&tcpConfig);

    struct SolidSyslogSender* tlsSender = BddTargetTlsSender_Create(resolver, false);

    static struct SolidSyslogSender* inners[BDD_TARGET_SWITCH_COUNT];
    inners[BDD_TARGET_SWITCH_UDP] = udpSender;
    inners[BDD_TARGET_SWITCH_TCP] = tcpSender;
    inners[BDD_TARGET_SWITCH_TLS] = tlsSender;
    struct SolidSyslogSwitchingSenderConfig switchConfig = {
        .Senders = inners,
        .SenderCount = BDD_TARGET_SWITCH_COUNT,
        .Selector = BddTargetSwitchConfig_Selector,
    };
    BddTargetSwitchConfig_SetByName("udp");
    switchingSender = SolidSyslogSwitchingSender_Create(&switchConfig);
    return switchingSender;
}

/* Reverse-order teardown of the LwipRaw sender stack. BddTargetTlsSender owns
 * the inner MbedTlsStream + LwipRawTcpStream + StreamSender pool slots, so it is
 * released before the plain-TCP tcpSender / tcpStream. */
static void TeardownNetwork(void)
{
    SolidSyslogSwitchingSender_Destroy(switchingSender);
    BddTargetTlsSender_Destroy();
    SolidSyslogUdpSender_Destroy(udpSender);
    SolidSyslogStreamSender_Destroy(tcpSender);
    SolidSyslogLwipRawTcpStream_Destroy(tcpStream);
    SolidSyslogLwipRawAddress_Destroy(udpAddress);
    SolidSyslogLwipRawAddress_Destroy(tcpAddress);
    SolidSyslogLwipRawDatagram_Destroy(datagram);
    SolidSyslogLwipRawDnsResolver_Destroy(resolver);
}
