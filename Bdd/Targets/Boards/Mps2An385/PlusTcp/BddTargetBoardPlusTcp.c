/* The QEMU mps2-an385 LAN9118 Ethernet controller under FreeRTOS-Plus-TCP -
 * see BddTargetBoardPlusTcp.h. The network interface and the controller driver
 * under it are upstream's, from Plus-TCP's MPS2_AN385 port. */

#include "BddTargetBoardPlusTcp.h"

#include <stdint.h>

/* IRQ number of the LAN9118 on this machine. The upstream NetworkInterface.c
 * enables ISER for this IRQ but does NOT write IPR - leaving the priority at
 * the reset default of 0, which is numerically more urgent than
 * configMAX_SYSCALL_INTERRUPT_PRIORITY and trips configASSERT the first time
 * the ISR calls a FreeRTOS API. So it is set here, before FreeRTOS_IPInit_Multi
 * triggers the interface init that flips ISER. */
#define ETHERNET_IRQ_NUMBER 13U

/* NVIC IPR (Interrupt Priority Register) base - one byte per IRQ. */
#define NVIC_IPR_BASE_ADDRESS UINT32_C(0xE000E400)

/* NVIC IPR is 8-bit per IRQ but only the top configPRIO_BITS are implemented.
 * Derive from the FreeRTOSConfig macros so a kernel-config change can't silently
 * leave IRQ 13 at a higher-than-syscall-safe priority. */
#define ETHERNET_IRQ_PRIORITY ((uint8_t) (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8U - configPRIO_BITS)))

extern NetworkInterface_t* pxMPS2_FillInterfaceDescriptor(BaseType_t xEMACIndex, NetworkInterface_t* pxInterface);

static void SetEthernetIrqPriority(void);

NetworkInterface_t* BddTargetBoardPlusTcp_FillInterface(NetworkInterface_t* networkInterface)
{
    SetEthernetIrqPriority();
    return pxMPS2_FillInterfaceDescriptor(0, networkInterface);
}

static void SetEthernetIrqPriority(void)
{
    // NOLINTNEXTLINE(performance-no-int-to-ptr) -- writing the NVIC IPR byte for IRQ 13.
    volatile uint8_t* ipr = (volatile uint8_t*) (NVIC_IPR_BASE_ADDRESS + ETHERNET_IRQ_NUMBER);
    *ipr = ETHERNET_IRQ_PRIORITY;
}
