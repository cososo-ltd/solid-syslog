#ifndef SOLIDSYSLOG_TESTS_FREERTOSFAKES_PORTMACRO_H
#define SOLIDSYSLOG_TESTS_FREERTOSFAKES_PORTMACRO_H

// NOLINTBEGIN(bugprone-macro-parentheses) -- FreeRTOS-Kernel port API:
// the function-declaration macros can't parenthesise their args.

/* Stub of FreeRTOS-Kernel's portmacro.h.
 *
 * The real kernel ships one portmacro.h per port × compiler combination
 * (e.g. portable/GCC/ARM_CM3/, portable/IAR/ARM_CM3/, portable/ThirdParty/
 * GCC/Posix/). Tests must stay independent of which port and which compiler
 * the integrator's production build will use, so FreeRtosFakes provides
 * this stub with just enough typedefs and macros for FreeRTOS.h and the
 * Plus-TCP headers to parse on a host C/C++ compiler.
 *
 * No scheduler runs; critical-section / yield / interrupt macros expand to
 * nothing, and tests never observe context-switch behaviour. The stack-type
 * choice doesn't matter — fakes never allocate task stacks.
 */

#include <stdint.h>

/* Kernel scalar types. BaseType_t / UBaseType_t are typedef'd to long /
 * unsigned long so they match the host compiler's long width (64-bit on x86_64
 * Linux, 32-bit on a 32-bit host) — that is intentional: the adapter under
 * test never depends on these widths since it stores BaseType_t args verbatim
 * and forwards them to the fake, and all FreeRTOS socket constants are small
 * integers that fit any width. Real targets bring their own portmacro.h. */
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;
typedef uint32_t StackType_t;

/* Mandatory port constants referenced by FreeRTOS.h / list.h / task.h. */
#define portMAX_DELAY ((TickType_t) 0xFFFFFFFFUL)
#define portSTACK_GROWTH (-1)
#define portTICK_PERIOD_MS ((TickType_t) 1000 / configTICK_RATE_HZ)
#define portBYTE_ALIGNMENT 8
#define portTICK_TYPE_IS_ATOMIC 1
#define portPOINTER_SIZE_TYPE intptr_t

/* Critical-section / interrupt / yield primitives — no-op for host tests. */
#define portYIELD() ((void) 0)
#define portYIELD_FROM_ISR(x) ((void) (x))
#define portENTER_CRITICAL() ((void) 0)
#define portEXIT_CRITICAL() ((void) 0)
#define portDISABLE_INTERRUPTS() ((void) 0)
#define portENABLE_INTERRUPTS() ((void) 0)
#define portSET_INTERRUPT_MASK_FROM_ISR() ((UBaseType_t) 0)
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x) ((void) (x))

/* Task-function declaration macro used in user-supplied tasks. The adapter
 * never declares a task, so this is here only to satisfy FreeRTOS.h's parse. */
#define portTASK_FUNCTION_PROTO(vFunction, pvParameters) void vFunction(void* pvParameters)
#define portTASK_FUNCTION(vFunction, pvParameters) void vFunction(void* pvParameters)

// NOLINTEND(bugprone-macro-parentheses)

#endif /* SOLIDSYSLOG_TESTS_FREERTOSFAKES_PORTMACRO_H */
