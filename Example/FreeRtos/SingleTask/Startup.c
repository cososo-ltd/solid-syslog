/* Cortex-M3 startup for the QEMU mps2-an385 machine — variant for the
 * FreeRTOS-Plus-TCP single-task example.
 *
 * Differs from Example/FreeRtos/Common/startup.c by extending the vector
 * table with the Cortex-M3 external IRQ entries (IRQ0..31). IRQ 13 is the
 * SMSC9220 (LAN9118) Ethernet controller in QEMU's mps2-an385 model and is
 * routed to EthernetISR (defined by the upstream Plus-TCP MPS2_AN385
 * NetworkInterface.c). All other IRQ slots fall through to Default_Handler. */

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

void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));

void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

/* Provided by the Plus-TCP MPS2_AN385 NetworkInterface.c — drained by the
 * EMAC RX task on each task notification. */
void EthernetISR(void) __attribute__((weak, alias("Default_Handler")));

void Reset_Handler(void)
{
    uint32_t* src = &_sidata;
    uint32_t* dst = &_sdata;
    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    dst = &_sbss;
    while (dst < &_ebss)
    {
        *dst++ = 0;
    }

    __libc_init_array();

    (void) main();

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

/* Cortex-M3 vector table. System exceptions (entries 0..15) followed by 32
 * external IRQs. Only IRQ 13 is wired to a real handler — every other slot
 * traps via Default_Handler so a stray interrupt is debuggable rather than
 * silently jumping into 0xFFFFFFFF. */
__attribute__((section(".vectors"), used)) const uint32_t vector_table[] = {
    (uint32_t) &_estack,           /* 0x00 — initial stack pointer */
    (uint32_t) Reset_Handler,      /* 0x04 — reset                 */
    (uint32_t) NMI_Handler,        /* 0x08                         */
    (uint32_t) HardFault_Handler,  /* 0x0C                         */
    (uint32_t) MemManage_Handler,  /* 0x10                         */
    (uint32_t) BusFault_Handler,   /* 0x14                         */
    (uint32_t) UsageFault_Handler, /* 0x18                         */
    0U, 0U, 0U, 0U,                /* 0x1C-0x28 reserved           */
    (uint32_t) SVC_Handler,        /* 0x2C — FreeRTOS              */
    (uint32_t) DebugMon_Handler,   /* 0x30                         */
    0U,                            /* 0x34 reserved                */
    (uint32_t) PendSV_Handler,     /* 0x38 — FreeRTOS              */
    (uint32_t) SysTick_Handler,    /* 0x3C — FreeRTOS              */

    /* External interrupts — IRQ0..IRQ31. IRQ 13 = LAN9118 Ethernet. */
    (uint32_t) Default_Handler, /* IRQ  0 — UART0 RX  */
    (uint32_t) Default_Handler, /* IRQ  1 — UART0 TX  */
    (uint32_t) Default_Handler, /* IRQ  2 — UART1 RX  */
    (uint32_t) Default_Handler, /* IRQ  3 — UART1 TX  */
    (uint32_t) Default_Handler, /* IRQ  4 — UART2 RX  */
    (uint32_t) Default_Handler, /* IRQ  5 — UART2 TX  */
    (uint32_t) Default_Handler, /* IRQ  6 — GPIO0     */
    (uint32_t) Default_Handler, /* IRQ  7 — GPIO1     */
    (uint32_t) Default_Handler, /* IRQ  8 — Timer0    */
    (uint32_t) Default_Handler, /* IRQ  9 — Timer1    */
    (uint32_t) Default_Handler, /* IRQ 10 — DualTimer */
    (uint32_t) Default_Handler, /* IRQ 11 — SPI0/1    */
    (uint32_t) Default_Handler, /* IRQ 12 — UART overflow */
    (uint32_t) EthernetISR,     /* IRQ 13 — Ethernet (LAN9118) */
    (uint32_t) Default_Handler, /* IRQ 14 — Touchscreen */
    (uint32_t) Default_Handler, /* IRQ 15 — Audio I2S */
    (uint32_t) Default_Handler, /* IRQ 16 */
    (uint32_t) Default_Handler, /* IRQ 17 */
    (uint32_t) Default_Handler, /* IRQ 18 */
    (uint32_t) Default_Handler, /* IRQ 19 */
    (uint32_t) Default_Handler, /* IRQ 20 */
    (uint32_t) Default_Handler, /* IRQ 21 */
    (uint32_t) Default_Handler, /* IRQ 22 */
    (uint32_t) Default_Handler, /* IRQ 23 */
    (uint32_t) Default_Handler, /* IRQ 24 */
    (uint32_t) Default_Handler, /* IRQ 25 */
    (uint32_t) Default_Handler, /* IRQ 26 */
    (uint32_t) Default_Handler, /* IRQ 27 */
    (uint32_t) Default_Handler, /* IRQ 28 */
    (uint32_t) Default_Handler, /* IRQ 29 */
    (uint32_t) Default_Handler, /* IRQ 30 */
    (uint32_t) Default_Handler, /* IRQ 31 */
};
