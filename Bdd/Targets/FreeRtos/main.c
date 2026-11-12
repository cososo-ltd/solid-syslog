/* FreeRTOS-Plus-TCP SolidSyslog BDD target for QEMU mps2-an385.
 *
 * The platform-independent pipeline - SolidSyslog lifecycle, file-backed store
 * + security policies, SD set, the interactive `set` handler, the Service drain
 * task, and the console glue - lives in Bdd/Targets/Common/BddTargetFreeRtosPipeline
 * (shared with the lwIP target, S29.03). This file keeps only the FreeRTOS-Plus-TCP
 * network backend and the FreeRTOS-Plus-FAT store behind the pipeline seam:
 * static-IP bring-up, the LAN9118 IRQ priority fix, the per-endpoint RNG /
 * sequence hooks, the PlusTcp sender wiring (UDP datagram + octet-framed TCP +
 * TLS/mTLS via mbedTLS over PlusTcp TCP), the RFC 5424 HOSTNAME read from the
 * Plus-TCP endpoint, and the Plus-FAT FS-mount seam (BddTargetPlusFatMount over
 * the FF_Disk_t media driver on the board's disk). The lwIP target pairs lwIP with
 * ChaN-FatFs instead - together the two targets prove the SolidSyslogFile seam
 * is FS-vendor-portable (S29.05).
 *
 * Static IPv4 (10.0.2.15) on the QEMU slirp network with the host reachable at
 * the slirp gateway 10.0.2.2. */

#include "BddTargetBoard.h"
#include "BddTargetBoardConfig.h"
#include "BddTargetBoardPlusTcp.h"
#include "BddTargetFreeRtosPipeline.h"
#include "BddTargetMtlsConfig.h"
#include "BddTargetOsPrimitives.h"
#include "BddTargetPlusFatMount.h"
#include "BddTargetSwitchConfig.h"
#include "BddTargetTlsConfig.h"
#include "BddTargetTlsSender.h"

#include "SolidSyslogHeaderField.h"
#include "SolidSyslogPlusTcpAddress.h"
#include "SolidSyslogPlusTcpDatagram.h"
#include "SolidSyslogPlusTcpResolver.h"
#include "SolidSyslogPlusTcpTcpStream.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogUdpSender.h"

#include <FreeRTOS.h>
#include <task.h>

#include <FreeRTOS_IP.h>
#include <FreeRTOS_Routing.h>
#include <FreeRTOS_Sockets.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Static IPv4 wiring on the board's network - see BddTargetBoardConfig.h. No
 * DHCP server is required. */
static const uint8_t TEST_IP_ADDRESS[ipIP_ADDRESS_LENGTH_BYTES] = {BDD_TARGET_BOARD_IP_OCTETS};
static const uint8_t TEST_NETMASK[ipIP_ADDRESS_LENGTH_BYTES] = {BDD_TARGET_BOARD_NETMASK_OCTETS};
static const uint8_t TEST_GATEWAY[ipIP_ADDRESS_LENGTH_BYTES] = {BDD_TARGET_BOARD_GATEWAY_OCTETS};
static const uint8_t TEST_DNS[ipIP_ADDRESS_LENGTH_BYTES] = {BDD_TARGET_BOARD_DNS_OCTETS};
static const uint8_t TEST_MAC[ipMAC_ADDRESS_LENGTH_BYTES] = {BDD_TARGET_BOARD_MAC_OCTETS};

/* Plus-TCP requires the network interface descriptor and its endpoint(s) to
 * outlive the IP stack. */
static NetworkInterface_t networkInterface;
static NetworkEndPoint_t networkEndPoint;

/* PlusTcp sender adapters - built by BuildSender on the interactive task, torn
 * down by TeardownNetwork. */
static struct SolidSyslogResolver* resolver = NULL;
static struct SolidSyslogDatagram* datagram = NULL;
static struct SolidSyslogAddress* udpAddress = NULL;
static struct SolidSyslogSender* udpSender = NULL;
static struct SolidSyslogStream* tcpStream = NULL;
static struct SolidSyslogAddress* tcpAddress = NULL;
static struct SolidSyslogSender* tcpSender = NULL;
static struct SolidSyslogSender* tlsSender = NULL;
static struct SolidSyslogSender* switchingSender = NULL;

/* Ensures the interactive + service tasks are created exactly once even if the
 * network goes down and back up. */
static BaseType_t interactiveTaskCreated = pdFALSE;

/* This target spawns its threads through the kernel API rather than the OS seam
 * in BddTargetOsPrimitives.h, because the rollback below needs the handle that
 * seam deliberately does not hand back. FreeRTOS-Plus-TCP pins it to this kernel
 * anyway - its own network event hook is what calls this. So convert the shared
 * byte sizes back into the stack words xTaskCreate wants. */
#define BDD_TARGET_STACK_WORDS(bytes) ((configSTACK_DEPTH_TYPE) ((bytes) / sizeof(StackType_t)))

static void GetHostname(struct SolidSyslogHeaderField* field, void* context);
static struct SolidSyslogSender* BuildSender(void);
static void TeardownNetwork(void);

static const struct BddTargetFreeRtosPipelineConfig PIPELINE_CONFIG = {
    .DefaultHost = BDD_TARGET_BOARD_ORACLE_ADDRESS,
    .BuildSender = BuildSender,
    .GetHostname = GetHostname,
    .TeardownNetwork = TeardownNetwork,
    .MountStore = BddTargetPlusFatMount_Mount,
    .UnmountStore = BddTargetPlusFatMount_Unmount,
    .CreateStoreFile = BddTargetPlusFatMount_CreateFile,
    .DestroyStoreFile = BddTargetPlusFatMount_DestroyFile,
};

int main(void)
{
    BddTargetBoard_Init();
    BddTargetFreeRtosPipeline_SetConfig(&PIPELINE_CONFIG);

    (void) BddTargetBoardPlusTcp_FillInterface(&networkInterface);
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

    /* The console lock has to exist before the IP task's event hook spawns the
     * two writers. */
    BddTargetOsPrimitives_InitialiseOs();

    vTaskStartScheduler();

    for (;;)
    {
    }
    return 0;
}

void vApplicationIPNetworkEventHook_Multi(eIPCallbackEvent_t eNetworkEvent, struct xNetworkEndPoint* pxEndPoint)
{
    (void) pxEndPoint;
    if ((eNetworkEvent == eNetworkUp) && (interactiveTaskCreated == pdFALSE))
    {
        TaskHandle_t interactiveTask = NULL;
        if (xTaskCreate(
                BddTargetFreeRtosPipeline_InteractiveTask,
                "interactive",
                BDD_TARGET_STACK_WORDS(BDD_TARGET_INTERACTIVE_STACK_BYTES),
                NULL,
                tskIDLE_PRIORITY + 1,
                &interactiveTask
            ) == pdPASS)
        {
            if (xTaskCreate(
                    BddTargetFreeRtosPipeline_ServiceTask,
                    "service",
                    BDD_TARGET_STACK_WORDS(BDD_TARGET_SERVICE_STACK_BYTES),
                    NULL,
                    tskIDLE_PRIORITY + 1,
                    NULL
                ) == pdPASS)
            {
                interactiveTaskCreated = pdTRUE;
            }
            else
            {
                /* Service create failed - undo the interactive task so the next
                 * eNetworkUp retries both cleanly instead of latching a
                 * half-started pipeline. Safe to delete: this hook runs on the
                 * higher-priority IP task, so the idle+1 interactive task has not
                 * been scheduled yet. */
                vTaskDelete(interactiveTask);
            }
        }
    }
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

/* Plus-TCP requires the application to provide a per-endpoint random seed source
 * for ARP / IP-ID generation. The QEMU mps2-an385 has no RNG; for a
 * deterministic smoke test we return the run-time tick mixed with a constant. */
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

static void GetHostname(struct SolidSyslogHeaderField* field, void* context)
{
    /* RFC 5424 section 6.2.4 rung 2 (static IP address). Read back from the IP stack so
     * a future DHCP / hostname slice satisfies the same rung without re-touching
     * this callback. */
    uint32_t ipAddress = 0U;
    char ipBuffer[16];
    (void) context;
    FreeRTOS_GetEndPointConfiguration(&ipAddress, NULL, NULL, NULL, &networkEndPoint);
    FreeRTOS_inet_ntoa(ipAddress, ipBuffer);
    SolidSyslogHeaderField_PrintUsAscii(field, ipBuffer, strlen(ipBuffer));
}

/* Build the PlusTcp SwitchingSender: UDP datagram, octet-framed TCP, and a
 * TLS/mTLS slot (mbedTLS over a second PlusTcp TCP stream). Default transport
 * UDP so existing @udp scenarios stay green; `set transport <udp|tcp|tls|mtls>`
 * flips it at runtime. Runs on the interactive task. */
static struct SolidSyslogSender* BuildSender(void)
{
    /* Route TLS / mTLS to the oracle by the address the board reaches it at,
     * the same path UDP/TCP take, since this target resolves no name for it.
     * ServerName is pinned to "syslog-ng" (the cert subject) so SNI / cert
     * verification still pass. */
    BddTargetTlsConfig_SetHost(BDD_TARGET_BOARD_ORACLE_ADDRESS);
    BddTargetTlsConfig_SetServerName("syslog-ng");
    BddTargetMtlsConfig_SetHost(BDD_TARGET_BOARD_ORACLE_ADDRESS);
    BddTargetMtlsConfig_SetServerName("syslog-ng");

    resolver = SolidSyslogPlusTcpResolver_Create();
    datagram = SolidSyslogPlusTcpDatagram_Create(NULL);
    udpAddress = SolidSyslogPlusTcpAddress_Create();
    struct SolidSyslogUdpSenderConfig udpConfig = {
        .Resolver = resolver,
        .Datagram = datagram,
        .Address = udpAddress,
        .Endpoint = BddTargetFreeRtosPipeline_GetEndpoint,
        .EndpointVersion = BddTargetFreeRtosPipeline_GetEndpointVersion,
    };
    udpSender = SolidSyslogUdpSender_Create(&udpConfig);

    tcpStream = SolidSyslogPlusTcpTcpStream_Create(NULL);
    tcpAddress = SolidSyslogPlusTcpAddress_Create();
    struct SolidSyslogStreamSenderConfig tcpConfig = {
        .Resolver = resolver,
        .Stream = tcpStream,
        .Address = tcpAddress,
        .Endpoint = BddTargetFreeRtosPipeline_GetEndpoint,
        .EndpointVersion = BddTargetFreeRtosPipeline_GetEndpointVersion,
    };
    tcpSender = SolidSyslogStreamSender_Create(&tcpConfig);

    tlsSender = BddTargetTlsSender_Create(resolver, false);

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

/* Reverse-order teardown of the PlusTcp sender stack. BddTargetTlsSender owns
 * the inner MbedTlsStream + PlusTcpTcpStream + StreamSender pool slots, so it is
 * released before the plain-TCP tcpSender / tcpStream. */
static void TeardownNetwork(void)
{
    SolidSyslogSwitchingSender_Destroy(switchingSender);
    BddTargetTlsSender_Destroy();
    tlsSender = NULL;
    SolidSyslogStreamSender_Destroy(tcpSender);
    SolidSyslogPlusTcpAddress_Destroy(tcpAddress);
    SolidSyslogPlusTcpTcpStream_Destroy(tcpStream);
    SolidSyslogUdpSender_Destroy(udpSender);
    SolidSyslogPlusTcpAddress_Destroy(udpAddress);
    SolidSyslogPlusTcpDatagram_Destroy(datagram);
    SolidSyslogPlusTcpResolver_Destroy(resolver);
}
