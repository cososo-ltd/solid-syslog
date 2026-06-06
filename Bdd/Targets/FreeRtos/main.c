/* FreeRTOS-Plus-TCP SolidSyslog BDD target for QEMU mps2-an385.
 *
 * The platform-independent pipeline — SolidSyslog lifecycle, file-backed store
 * + security policies, SD set, the interactive `set` handler, the Service drain
 * task, and the console glue — lives in Bdd/Targets/Common/BddTargetFreeRtosPipeline
 * (shared with the lwIP target, S29.03). This file keeps only the FreeRTOS-Plus-TCP
 * network backend and the FreeRTOS-Plus-FAT store behind the pipeline seam:
 * static-IP bring-up, the LAN9118 IRQ priority fix, the per-endpoint RNG /
 * sequence hooks, the PlusTcp sender wiring (UDP datagram + octet-framed TCP +
 * TLS/mTLS via mbedTLS over PlusTcp TCP), the RFC 5424 HOSTNAME read from the
 * Plus-TCP endpoint, and the Plus-FAT FS-mount seam (BddTargetPlusFatMount over
 * the FF_Disk_t semihosting media driver). The lwIP target pairs lwIP with
 * ChaN-FatFs instead — together the two targets prove the SolidSyslogFile seam
 * is FS-vendor-portable (S29.05).
 *
 * Static IPv4 (10.0.2.15) on the QEMU slirp network with the host reachable at
 * the slirp gateway 10.0.2.2. */

#include "BddTargetFreeRtosPipeline.h"
#include "BddTargetMtlsConfig.h"
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

#define CMSDK_UART0_BASE_ADDRESS UINT32_C(0x40004000)

/* IRQ number for the QEMU mps2-an385 LAN9118 Ethernet controller. The upstream
 * Plus-TCP NetworkInterface.c enables ISER for this IRQ but does NOT write IPR —
 * leaving the priority at the reset default of 0, which is numerically more
 * urgent than configMAX_SYSCALL_INTERRUPT_PRIORITY and trips configASSERT the
 * first time the ISR calls a FreeRTOS API. We set IPR explicitly here before
 * FreeRTOS_IPInit_Multi triggers the interface init that flips ISER. */
#define ETHERNET_IRQ_NUMBER 13U

/* NVIC IPR (Interrupt Priority Register) base — one byte per IRQ. */
#define NVIC_IPR_BASE_ADDRESS UINT32_C(0xE000E400)

/* NVIC IPR is 8-bit per IRQ but only the top configPRIO_BITS are implemented.
 * Derive from the FreeRTOSConfig macros so a kernel-config change can't silently
 * leave IRQ 13 at a higher-than-syscall-safe priority. */
#define ETHERNET_IRQ_PRIORITY ((uint8_t) (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8U - configPRIO_BITS)))

/* Static IPv4 wiring matching the QEMU slirp default. 10.0.2.15 is the standard
 * slirp DHCP-allocated guest address; we hardcode it so no DHCP server is
 * required. 10.0.2.2 is the slirp gateway routed to the QEMU host; 10.0.2.3 is
 * slirp's built-in DNS forwarder. */
static const uint8_t TEST_IP_ADDRESS[ipIP_ADDRESS_LENGTH_BYTES] = {10U, 0U, 2U, 15U};
static const uint8_t TEST_NETMASK[ipIP_ADDRESS_LENGTH_BYTES] = {255U, 255U, 255U, 0U};
static const uint8_t TEST_GATEWAY[ipIP_ADDRESS_LENGTH_BYTES] = {10U, 0U, 2U, 2U};
static const uint8_t TEST_DNS[ipIP_ADDRESS_LENGTH_BYTES] = {10U, 0U, 2U, 3U};
static const uint8_t TEST_MAC[ipMAC_ADDRESS_LENGTH_BYTES] = {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U};

/* Plus-TCP requires the network interface descriptor and its endpoint(s) to
 * outlive the IP stack. */
static NetworkInterface_t networkInterface;
static NetworkEndPoint_t networkEndPoint;

/* PlusTcp sender adapters — built by BuildSender on the interactive task, torn
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

extern NetworkInterface_t* pxMPS2_FillInterfaceDescriptor(BaseType_t xEMACIndex, NetworkInterface_t* pxInterface);

static void SetEthernetIrqPriority(void);
static void GetHostname(struct SolidSyslogHeaderField* field, void* context);
static struct SolidSyslogSender* BuildSender(void);
static void TeardownNetwork(void);

static const struct BddTargetFreeRtosPipelineConfig PIPELINE_CONFIG = {
    .DefaultHost = "10.0.2.2",
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
    BddTargetFreeRtosPipeline_InitConsole(CMSDK_UART0_BASE_ADDRESS);
    BddTargetFreeRtosPipeline_SetConfig(&PIPELINE_CONFIG);

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

void vApplicationIPNetworkEventHook_Multi(eIPCallbackEvent_t eNetworkEvent, struct xNetworkEndPoint* pxEndPoint)
{
    (void) pxEndPoint;
    if ((eNetworkEvent == eNetworkUp) && (interactiveTaskCreated == pdFALSE))
    {
        TaskHandle_t interactiveTask = NULL;
        if (xTaskCreate(
                BddTargetFreeRtosPipeline_InteractiveTask,
                "interactive",
                configMINIMAL_STACK_SIZE * BDD_TARGET_INTERACTIVE_STACK_MULTIPLIER,
                NULL,
                tskIDLE_PRIORITY + 1,
                &interactiveTask
            ) == pdPASS)
        {
            if (xTaskCreate(
                    BddTargetFreeRtosPipeline_ServiceTask,
                    "service",
                    configMINIMAL_STACK_SIZE * BDD_TARGET_SERVICE_STACK_MULTIPLIER,
                    NULL,
                    tskIDLE_PRIORITY + 1,
                    NULL
                ) == pdPASS)
            {
                interactiveTaskCreated = pdTRUE;
            }
            else
            {
                /* Service create failed — undo the interactive task so the next
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

static void SetEthernetIrqPriority(void)
{
    // NOLINTNEXTLINE(performance-no-int-to-ptr) -- writing the NVIC IPR byte for IRQ 13.
    volatile uint8_t* ipr = (volatile uint8_t*) (NVIC_IPR_BASE_ADDRESS + ETHERNET_IRQ_NUMBER);
    *ipr = ETHERNET_IRQ_PRIORITY;
}

static void GetHostname(struct SolidSyslogHeaderField* field, void* context)
{
    /* RFC 5424 §6.2.4 rung 2 (static IP address). Read back from the IP stack so
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
    /* Route TLS / mTLS via the slirp gateway 10.0.2.2 (same path UDP/TCP take —
     * slirp DNS doesn't reach the docker alias in CI). ServerName is pinned to
     * "syslog-ng" (the cert subject) so SNI / cert verification still pass. */
    BddTargetTlsConfig_SetHost("10.0.2.2");
    BddTargetTlsConfig_SetServerName("syslog-ng");
    BddTargetMtlsConfig_SetHost("10.0.2.2");
    BddTargetMtlsConfig_SetServerName("syslog-ng");

    resolver = SolidSyslogPlusTcpResolver_Create();
    datagram = SolidSyslogPlusTcpDatagram_Create();
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
