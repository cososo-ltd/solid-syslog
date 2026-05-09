/* FreeRTOS-Plus-TCP single-task SolidSyslog example for QEMU mps2-an385.
 *
 * Slice 3b.2 of S08.03 — replaces 3b.1's hardcoded "ping" smoke task with
 * a real SolidSyslog wiring. Static IPv4 (10.0.2.15) on the QEMU slirp
 * network with the host reachable at the slirp gateway 10.0.2.2; a single
 * FreeRTOS task runs Example/Common/ExampleInteractive over qemu -serial
 * stdio (CmsdkUart RX wired into newlib's _read in Example/FreeRtos/
 * Common/Syscalls.c). On link-up the IP-task event hook spawns the
 * interactive task once; SolidSyslog is configured with a NullBuffer +
 * UdpSender driving the slice-1 SolidSyslogFreeRtosDatagram via the
 * slice-3a static resolver, so each `send N` line over the UART emits N
 * RFC 5424 datagrams to {10.0.2.2, 5514}. */

#include "CmsdkUart.h"
#include "ExampleInteractive.h"
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogFreeRtosDatagram.h"
#include "SolidSyslogFreeRtosStaticResolver.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogUdpSender.h"

#include <FreeRTOS.h>
#include <task.h>

#include <FreeRTOS_IP.h>
#include <FreeRTOS_Routing.h>

#include <stdint.h>
#include <stdio.h>

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
#define EXAMPLE_UDP_PORT 5514U

/* SolidSyslog_Log allocates two char[SOLIDSYSLOG_MAX_MESSAGE_SIZE] frames
 * (~4 KB) plus formatter storage on its formatter path; ExampleInteractive
 * adds a 256-byte fgets line and newlib printf (~1 KB). Empirically the
 * task hard-faults at *16 (8 KB) once the SolidSyslog setup runs — newlib
 * printf and the formatter path together exceed that budget. *32 (16 KB)
 * keeps the smoke run stable; a follow-up will introduce CMake-driven
 * memory scaling once the budget is properly characterised. The task only
 * exists in this single-task example, so heap_4 (96 KB) absorbs it. */
#define INTERACTIVE_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 32U)

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

/* Walking-skeleton TEST_* values — slice-4 will replace these with config
 * injected over the interactive command grammar. */
static const char TEST_HOSTNAME[]   = "FreeRtosExample";
static const char TEST_APP_NAME[]   = "SolidSyslogExample";
static const char TEST_PROCESS_ID[] = "1";
static const char TEST_MESSAGE_ID[] = "example";
static const char TEST_MESSAGE[]    = "Hello from FreeRTOS";

/* RFC 5424 publication date — placeholder until S08.03 slice 4+ injects a
 * real RTC-backed clock callback. */
static const struct SolidSyslogTimestamp TEST_TIMESTAMP = {
    .year             = 2009U,
    .month            = 3U,
    .day              = 23U,
    .hour             = 0U,
    .minute           = 0U,
    .second           = 0U,
    .microsecond      = 0U,
    .utcOffsetMinutes = 0,
};

/* Plus-TCP requires the network interface descriptor and its endpoint(s)
 * to outlive the IP stack. */
static NetworkInterface_t networkInterface;
static NetworkEndPoint_t  networkEndPoint;

static SolidSyslogFreeRtosStaticResolverStorage resolverStorage;
static SolidSyslogFreeRtosDatagramStorage       datagramStorage;

/* Ensures the interactive task is created exactly once even if the network
 * goes down and back up. */
static BaseType_t interactiveTaskCreated = pdFALSE;

extern NetworkInterface_t* pxMPS2_FillInterfaceDescriptor(BaseType_t xEMACIndex, NetworkInterface_t* pxInterface);

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
    SolidSyslogFormatter_BoundedString(formatter, TEST_HOSTNAME, sizeof(TEST_HOSTNAME) - 1U);
}

static void GetAppName(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_BoundedString(formatter, TEST_APP_NAME, sizeof(TEST_APP_NAME) - 1U);
}

static void GetProcessId(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_BoundedString(formatter, TEST_PROCESS_ID, sizeof(TEST_PROCESS_ID) - 1U);
}

static void GetTimestamp(struct SolidSyslogTimestamp* timestamp)
{
    *timestamp = TEST_TIMESTAMP;
}

static void GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    /* SolidSyslogFreeRtosStaticResolver ignores the host string, so we
     * leave it empty; the port still needs to be populated for sendto. */
    SolidSyslogFormatter_BoundedString(endpoint->host, "", 0U);
    endpoint->port = (uint16_t) EXAMPLE_UDP_PORT;
}

static uint32_t GetEndpointVersion(void)
{
    return 0U;
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
    struct SolidSyslogSender* sender = SolidSyslogUdpSender_Create(&udpConfig);
    struct SolidSyslogBuffer* buffer = SolidSyslogNullBuffer_Create(sender);
    struct SolidSyslogStore*  store  = SolidSyslogNullStore_Create();

    struct SolidSyslogConfig config = {
        .buffer       = buffer,
        .sender       = NULL,
        .clock        = GetTimestamp,
        .getHostname  = GetHostname,
        .getAppName   = GetAppName,
        .getProcessId = GetProcessId,
        .store        = store,
        .sd           = NULL,
        .sdCount      = 0U,
    };
    SolidSyslog_Create(&config);

    struct SolidSyslogMessage message = {
        .facility  = SOLIDSYSLOG_FACILITY_LOCAL0,
        .severity  = SOLIDSYSLOG_SEVERITY_INFO,
        .messageId = TEST_MESSAGE_ID,
        .msg       = TEST_MESSAGE,
    };

    ExampleInteractive_Run(&message, stdin, NULL);

    SolidSyslog_Destroy();
    SolidSyslogNullStore_Destroy();
    SolidSyslogNullBuffer_Destroy();
    SolidSyslogUdpSender_Destroy();
    SolidSyslogFreeRtosDatagram_Destroy(datagram);
    SolidSyslogFreeRtosStaticResolver_Destroy(resolver);

    vTaskDelete(NULL);
}

void vApplicationIPNetworkEventHook_Multi(eIPCallbackEvent_t eNetworkEvent, struct xNetworkEndPoint* pxEndPoint)
{
    (void) pxEndPoint;
    if ((eNetworkEvent == eNetworkUp) && (interactiveTaskCreated == pdFALSE))
    {
        if (xTaskCreate(InteractiveTask, "interactive", INTERACTIVE_TASK_STACK_DEPTH, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS)
        {
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
