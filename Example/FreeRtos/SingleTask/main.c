/* FreeRTOS-Plus-TCP bring-up smoke test on QEMU mps2-an385 (Cortex-M3).
 *
 * Slice 3b.1 of S08.03: prove the IP stack initialises and a UDP datagram
 * escapes the guest via slirp. Static IPv4 (10.0.2.15), default gateway
 * (10.0.2.2 — the slirp gateway, routed to the host), no DHCP / DNS.
 *
 * On link-up the network event hook spawns a one-shot smoke task that
 *   - prints "network up\n" over the CMSDK UART (QEMU -serial stdio)
 *   - opens a UDP socket
 *   - sends "ping" to {10.0.2.2, 5514}
 *   - closes the socket and self-deletes.
 *
 * Slice 3b.2 replaces this scaffold with a SolidSyslogConfig + UdpSender
 * wiring that drives the slice-1 SolidSyslogFreeRtosDatagram adapter via
 * the slice-3a SolidSyslogFreeRtosStaticResolver. */

#include "CmsdkUart.h"

#include <FreeRTOS.h>
#include <task.h>

#include <FreeRTOS_ARP.h>
#include <FreeRTOS_IP.h>
#include <FreeRTOS_Routing.h>
#include <FreeRTOS_Sockets.h>

#include <stdint.h>
#include <stdio.h>
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

/* Static IPv4 wiring matching the QEMU slirp default. 10.0.2.15 is the
 * standard slirp DHCP-allocated guest address; we hardcode it here so no
 * DHCP server is required. */
static const uint8_t TEST_IP_ADDRESS[ipIP_ADDRESS_LENGTH_BYTES] = {10U, 0U, 2U, 15U};
static const uint8_t TEST_NETMASK[ipIP_ADDRESS_LENGTH_BYTES]    = {255U, 255U, 255U, 0U};
static const uint8_t TEST_GATEWAY[ipIP_ADDRESS_LENGTH_BYTES]    = {10U, 0U, 2U, 2U};
static const uint8_t TEST_DNS[ipIP_ADDRESS_LENGTH_BYTES]        = {10U, 0U, 2U, 3U};

/* Locally-administered MAC (U/L bit set, multicast bit clear). */
static const uint8_t TEST_MAC[ipMAC_ADDRESS_LENGTH_BYTES] = {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U};

#define TEST_TARGET_PORT 5514U
static const uint8_t TEST_PAYLOAD[] = {'p', 'i', 'n', 'g'};

/* Static storage for the network interface and its single endpoint. The
 * Plus-TCP API requires these to outlive the IP stack. */
static NetworkInterface_t networkInterface;
static NetworkEndPoint_t  networkEndPoint;

/* Ensures the smoke task is created exactly once even if the network goes
 * down and back up. */
static BaseType_t smokeTaskCreated = pdFALSE;

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
    vTaskDelay(pdMS_TO_TICKS((TickType_t) milliseconds));
}

static const CmsdkUartMemoryAccess MMIO_ACCESS = {MmioRead32, MmioWrite32, RtosSleep};

static void SetEthernetIrqPriority(void)
{
    // NOLINTNEXTLINE(performance-no-int-to-ptr) -- writing the NVIC IPR byte for IRQ 13.
    volatile uint8_t* ipr = (volatile uint8_t*) (NVIC_IPR_BASE_ADDRESS + ETHERNET_IRQ_NUMBER);
    *ipr                  = ETHERNET_IRQ_PRIORITY;
}

static void SmokeTask(void* argument)
{
    (void) argument;

    printf("network up\n");
    fflush(stdout);

    Socket_t socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
    if (socket != FREERTOS_INVALID_SOCKET)
    {
        struct freertos_sockaddr destination;
        memset(&destination, 0, sizeof(destination));
        destination.sin_len               = (uint8_t) sizeof(destination);
        destination.sin_family            = FREERTOS_AF_INET;
        destination.sin_port              = FreeRTOS_htons(TEST_TARGET_PORT);
        destination.sin_address.ulIP_IPv4 = FreeRTOS_inet_addr_quick(TEST_GATEWAY[0], TEST_GATEWAY[1], TEST_GATEWAY[2], TEST_GATEWAY[3]);

        /* Pre-resolve ARP for the gateway. Without this the first sendto
         * triggers ARP and the datagram is dropped while resolution is in
         * flight; under slirp ARP completes in well under 200 ms. */
        FreeRTOS_OutputARPRequest(destination.sin_address.ulIP_IPv4);
        vTaskDelay(pdMS_TO_TICKS(200));

        (void) FreeRTOS_sendto(socket, TEST_PAYLOAD, sizeof(TEST_PAYLOAD), 0, &destination, sizeof(destination));

        (void) FreeRTOS_closesocket(socket);
    }

    vTaskDelete(NULL);
}

void vApplicationIPNetworkEventHook_Multi(eIPCallbackEvent_t eNetworkEvent, struct xNetworkEndPoint* pxEndPoint)
{
    (void) pxEndPoint;
    if ((eNetworkEvent == eNetworkUp) && (smokeTaskCreated == pdFALSE))
    {
        if (xTaskCreate(SmokeTask, "smoke", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS)
        {
            smokeTaskCreated = pdTRUE;
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
 * deterministic smoke test we return the run-time tick mixed with the
 * endpoint pointer. */
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
