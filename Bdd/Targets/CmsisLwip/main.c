/* CMSIS-RTOS2 + lwIP (Sockets API, NO_SYS=0) SolidSyslog BDD target for QEMU
 * mps2-an385.
 *
 * This is the CmsisLwip target, and with S35.03 it links the stack it is named
 * for - see README.md.
 *
 * The platform-independent pipeline - SolidSyslog lifecycle, LittleFS-backed store
 * + security policies, SD set, the interactive `set` handler, the Service drain
 * task, and the console glue - lives in Bdd/Targets/Common/BddTargetFreeRtosPipeline
 * (shared with the FreeRTOS-Plus-TCP target, S29.03). This file keeps only the
 * lwIP network backend behind the pipeline seam: the tcpip thread, the
 * hand-written LAN9118 netif (netif/EthernetIf.c), the static-IP bring-up +
 * gateway ARP warm-up, the LwipSocket sender wiring (UDP + octet-framed TCP +
 * TLS/mTLS via mbedTLS over a second LwipSocket TCP), and the RFC 5424 HOSTNAME
 * read from the netif.
 *
 * There is no marshal seam here. The Sockets API is thread-safe, so every
 * adapter call runs on the task that made it; the only lwIP core touch this
 * file makes outside the tcpip thread is the ARP cache read below, which takes
 * the core lock itself.
 *
 * Static IPv4 (10.0.2.15) on the QEMU slirp network, host reachable at the slirp
 * gateway 10.0.2.2. The oracle is addressed by name ("syslog-ng") via
 * SolidSyslogLwipSocketResolver; lwIP's DNS_LOCAL_HOSTLIST (see lwipopts.h) maps
 * that name statically to 10.0.2.2 (slirp can't return a reachable address for
 * the docker alias over real DNS). */

#include "BddTargetLittleFsMount.h"
#include "BddTargetFreeRtosPipeline.h"
#include "BddTargetOsPrimitives.h"
#include "EthernetIf.h"

#include "BddTargetSwitchConfig.h"
#include "BddTargetTlsSender.h"

#include "SolidSyslogHeaderField.h"
#include "SolidSyslogLwipSocketAddress.h"
#include "SolidSyslogLwipSocketDatagram.h"
#include "SolidSyslogLwipSocketResolver.h"
#include "SolidSyslogLwipSocketTcpStream.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogUdpSender.h"

#include "lwip/etharp.h"
#include "lwip/init.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define CMSDK_UART0_BASE_ADDRESS UINT32_C(0x40004000)

/* lwIP netif descriptor - must outlive the tcpip thread. */
static struct netif networkInterface;
/* Gateway IP, kept at file scope so the ARP warm-up can reach it after bring-up. */
static ip4_addr_t gatewayAddress;

/* LwipSocket sender adapters - built by BuildSender on the interactive task,
 * torn down by TeardownNetwork. */
static struct SolidSyslogResolver* resolver = NULL;
static struct SolidSyslogDatagram* datagram = NULL;
static struct SolidSyslogAddress* udpAddress = NULL;
static struct SolidSyslogSender* udpSender = NULL;
static struct SolidSyslogStream* tcpStream = NULL;
static struct SolidSyslogAddress* tcpAddress = NULL;
static struct SolidSyslogSender* tcpSender = NULL;
static struct SolidSyslogSender* switchingSender = NULL;

static void NetworkBringUp(void* context);
static void WarmUpGatewayArp(void);
static bool GatewayIsResolved(void);
static void GetHostname(struct SolidSyslogHeaderField* field, void* context);
static struct SolidSyslogSender* BuildSender(void);
static void TeardownNetwork(void);

static const struct BddTargetFreeRtosPipelineConfig PIPELINE_CONFIG = {
    .DefaultHost = "syslog-ng",
    .BuildSender = BuildSender,
    .GetHostname = GetHostname,
    .TeardownNetwork = TeardownNetwork,
    .MountStore = BddTargetLittleFsMount_Mount,
    .UnmountStore = BddTargetLittleFsMount_Unmount,
    .CreateStoreFile = BddTargetLittleFsMount_CreateFile,
    .DestroyStoreFile = BddTargetLittleFsMount_DestroyFile,
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

    /* Create the tcpip thread + mbox + core-lock mutex. Pre-scheduler safe; the
     * thread runs once the scheduler starts. The netif bring-up is deferred to
     * NetworkBringUp on that thread (smsc9220_init calls vTaskDelay, which would
     * deref a NULL pxCurrentTCB before the scheduler). */
    tcpip_init(NULL, NULL);

    BddTargetOsPrimitives_InitialiseOs();

    if (!BddTargetOsPrimitives_Spawn(
            BddTargetFreeRtosPipeline_InteractiveTask,
            "interactive",
            BDD_TARGET_INTERACTIVE_STACK_BYTES
        ))
    {
        BddTargetFreeRtosPipeline_Exit(1);
    }
    if (!BddTargetOsPrimitives_Spawn(BddTargetFreeRtosPipeline_ServiceTask, "service", BDD_TARGET_SERVICE_STACK_BYTES))
    {
        BddTargetFreeRtosPipeline_Exit(1);
    }

    BddTargetOsPrimitives_StartScheduler();

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
     * the first datagram; an ARP miss on the first send would drop it. The reply
     * is processed by this tcpip thread; the interactive task waits via
     * WarmUpGatewayArp. */
    (void) etharp_request(&networkInterface, &gatewayAddress);
}

/* Blocks the calling (interactive) task - never the tcpip thread - until the
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
        LOCK_TCPIP_CORE();
        bool resolved = GatewayIsResolved();
        UNLOCK_TCPIP_CORE();
        if (resolved)
        {
            break;
        }
        BddTargetOsPrimitives_Sleep(WARM_UP_INTERVAL_MS);
    }
}

/* Whether the gateway's MAC is in the ARP cache yet. Reads lwIP core state, so
 * the caller holds the core lock. */
static bool GatewayIsResolved(void)
{
    struct eth_addr* ethRet = NULL;
    const ip4_addr_t* ipRet = NULL;
    return (etharp_find_addr(&networkInterface, &gatewayAddress, &ethRet, &ipRet) >= 0);
}

static void GetHostname(struct SolidSyslogHeaderField* field, void* context)
{
    /* RFC 5424 §6.2.4 rung 2 (static IP) - read back from the netif so a future
     * DHCP slice satisfies the same rung without touching this callback. */
    const char* address = ip4addr_ntoa(netif_ip4_addr(&networkInterface));
    (void) context;
    SolidSyslogHeaderField_PrintUsAscii(field, address, strlen(address));
}

/* Bring up the netif on the tcpip thread, warm the gateway ARP, then build the
 * LwipSocket SwitchingSender: UDP, octet-framed TCP, and a TLS/mTLS slot (mbedTLS
 * over a second LwipSocket TCP stream). Default transport UDP. Runs on the
 * interactive task - the adapters open sockets against a started lwIP core,
 * which is now up. */
static struct SolidSyslogSender* BuildSender(void)
{
    /* Bring the netif up on the tcpip thread now the scheduler is running, then
     * wait for the gateway ARP so the first datagram is not lost to a cache miss. */
    (void) tcpip_callback(NetworkBringUp, NULL);
    WarmUpGatewayArp();

    /* SolidSyslogLwipSocketResolver resolves the oracle by name; the
     * DNS_LOCAL_HOSTLIST entry maps "syslog-ng" -> 10.0.2.2 on the guest, so the
     * destination host equals the TLS serverName / cert subject without any
     * numeric pin. lwip_getaddrinfo blocks the calling task until lwIP answers,
     * which a hostlist hit does without leaving the guest - and this is the
     * interactive task, never the tcpip thread the resolve waits on. */
    resolver = SolidSyslogLwipSocketResolver_Create();
    datagram = SolidSyslogLwipSocketDatagram_Create();
    udpAddress = SolidSyslogLwipSocketAddress_Create();
    struct SolidSyslogUdpSenderConfig udpConfig = {
        .Resolver = resolver,
        .Datagram = datagram,
        .Address = udpAddress,
        .Endpoint = BddTargetFreeRtosPipeline_GetEndpoint,
        .EndpointVersion = BddTargetFreeRtosPipeline_GetEndpointVersion,
    };
    udpSender = SolidSyslogUdpSender_Create(&udpConfig);

    /* Plain TCP: RFC 6587 octet-framed StreamSender over the LwipSocket TCP
     * stream. NULL config leaves the connect deadline at the
     * SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable, which the stream bounds with
     * lwip_select rather than a sleep this target would have to supply. */
    tcpStream = SolidSyslogLwipSocketTcpStream_Create(NULL);
    tcpAddress = SolidSyslogLwipSocketAddress_Create();
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

/* Reverse-order teardown of the LwipSocket sender stack. BddTargetTlsSender owns
 * the inner MbedTlsStream + LwipSocketTcpStream + StreamSender pool slots, so it
 * is released before the plain-TCP tcpSender / tcpStream. */
static void TeardownNetwork(void)
{
    SolidSyslogSwitchingSender_Destroy(switchingSender);
    BddTargetTlsSender_Destroy();
    SolidSyslogUdpSender_Destroy(udpSender);
    SolidSyslogStreamSender_Destroy(tcpSender);
    SolidSyslogLwipSocketTcpStream_Destroy(tcpStream);
    SolidSyslogLwipSocketAddress_Destroy(udpAddress);
    SolidSyslogLwipSocketAddress_Destroy(tcpAddress);
    SolidSyslogLwipSocketDatagram_Destroy(datagram);
    SolidSyslogLwipSocketResolver_Destroy(resolver);
}
