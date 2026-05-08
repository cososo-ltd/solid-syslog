/* Cortex-M3 startup for the QEMU mps2-an385 machine.
 *
 * Provides the vector table, the Reset_Handler that initialises .data /
 * .bss and calls main(), and weak default handlers for unhandled
 * exceptions. The FreeRTOS GCC ARM_CM3 port supplies SVC_Handler,
 * PendSV_Handler, and SysTick_Handler (aliased via FreeRTOSConfig.h). */

#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

extern int  main(void);
extern void __libc_init_array(void);

void Reset_Handler(void);
void Default_Handler(void);

/* Cortex-M core exception handlers — weak so user code can override. */
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* Provided by FreeRTOS port.c; vector table references the CMSIS names that
 * FreeRTOSConfig.h aliases to. */
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

void Reset_Handler(void)
{
    /* Copy .data initial values from FLASH to SRAM. */
    uint32_t* src = &_sidata;
    uint32_t* dst = &_sdata;
    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    /* Zero .bss. */
    dst = &_sbss;
    while (dst < &_ebss)
    {
        *dst++ = 0;
    }

    /* Run static C/C++ initialisers (newlib calls into this). */
    __libc_init_array();

    (void) main();

    /* main() returned — trap. */
    for (;;)
    {
    }
}

void Default_Handler(void)
{
    for (;;)
    {
        __asm volatile("bkpt #0");
    }
}

/* Cortex-M3 vector table at the start of FLASH. */
__attribute__((section(".vectors"), used)) const uint32_t vector_table[] = {
    (uint32_t) &_estack,           /* 0x00 — initial stack pointer */
    (uint32_t) Reset_Handler,      /* 0x04 — reset                 */
    (uint32_t) NMI_Handler,        /* 0x08                         */
    (uint32_t) HardFault_Handler,  /* 0x0C                         */
    (uint32_t) MemManage_Handler,  /* 0x10                         */
    (uint32_t) BusFault_Handler,   /* 0x14                         */
    (uint32_t) UsageFault_Handler, /* 0x18                         */
    0U,
    0U,
    0U,
    0U,                          /* 0x1C-0x28 reserved           */
    (uint32_t) SVC_Handler,      /* 0x2C — FreeRTOS              */
    (uint32_t) DebugMon_Handler, /* 0x30                         */
    0U,                          /* 0x34 reserved                */
    (uint32_t) PendSV_Handler,   /* 0x38 — FreeRTOS              */
    (uint32_t) SysTick_Handler,  /* 0x3C — FreeRTOS              */
};
