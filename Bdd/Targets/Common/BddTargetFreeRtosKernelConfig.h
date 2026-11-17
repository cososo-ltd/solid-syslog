#ifndef BDD_TARGET_FREERTOS_KERNEL_CONFIG_H
#define BDD_TARGET_FREERTOS_KERNEL_CONFIG_H

/* The FreeRTOS kernel configuration that follows from the board and the board
 * seam, and not from the target. Each target's FreeRTOSConfig.h includes this,
 * so the interrupt priority ceiling for kernel calls, which the boards'
 * Ethernet set-up reads, and the assertion handler are defined once. */

#include "BddTargetBoard.h"
#include "BddTargetBoardConfig.h"

/* The NVIC priority bits the board implements - see BddTargetBoardConfig.h.
 * The kernel's own interrupts run at the least urgent priority the board has,
 * which is every implemented bit set. An interrupt that calls the kernel runs
 * at 5 or less urgent. */
#define configPRIO_BITS BDD_TARGET_BOARD_NVIC_PRIO_BITS
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY ((1 << configPRIO_BITS) - 1)
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* A failed assertion reports its file and line on the console and stops the
 * target. */
#define BDD_TARGET_TEXT_OF_(x) #x
#define BDD_TARGET_TEXT_OF(x) BDD_TARGET_TEXT_OF_(x)
#define configASSERT(x)                                                                          \
    do                                                                                           \
    {                                                                                            \
        if (!(x))                                                                                \
        {                                                                                        \
            BddTargetBoard_Fault("kernel assertion", __FILE__ ":" BDD_TARGET_TEXT_OF(__LINE__)); \
        }                                                                                        \
    } while (0)

#endif /* BDD_TARGET_FREERTOS_KERNEL_CONFIG_H */
