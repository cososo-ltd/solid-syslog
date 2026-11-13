#ifndef BDD_TARGET_CMSIS_DEVICE_H
#define BDD_TARGET_CMSIS_DEVICE_H

/* The CMSIS device header for QEMU mps2-an385, a Cortex-M3. CMSIS-FreeRTOS
 * includes it through CMSIS_device_header, which the CMSIS-RTOS2 target's
 * kernel half defines for the wrapper's source; outside a CMSIS-Pack RTE build
 * the wrapper refuses to compile without one. It reads only the core, so
 * nothing here goes beyond the core's exceptions. Every board supplies a
 * header of this name; on real hardware it wraps the vendor's device header. */

#include "BddTargetBoardConfig.h"

typedef enum IRQn
{
    NonMaskableInt_IRQn = -14,
    HardFault_IRQn = -13,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn = -11,
    UsageFault_IRQn = -10,
    SVCall_IRQn = -5,
    DebugMonitor_IRQn = -4,
    PendSV_IRQn = -2,
    SysTick_IRQn = -1
} IRQn_Type;

#define __CM3_REV 0x0201U
#define __MPU_PRESENT 1U
#define __VTOR_PRESENT 1U
#define __NVIC_PRIO_BITS BDD_TARGET_BOARD_NVIC_PRIO_BITS
#define __Vendor_SysTickConfig 0U

#include "core_cm3.h"

#endif /* BDD_TARGET_CMSIS_DEVICE_H */
