/* The QEMU mps2-an385 machine behind the board seam - see BddTargetBoard.h.
 *
 * The console is CMSDK UART0, which QEMU surfaces over `-serial stdio`, driven
 * polled by CmsdkUart. The run ends through ARM semihosting, which QEMU turns
 * into its own exit status. */

#include "BddTargetBoard.h"

#include "BddTargetOsPrimitives.h"
#include "CmsdkUart.h"

#include <stdint.h>
#include <string.h>

#define CMSDK_UART0_BASE_ADDRESS UINT32_C(0x40004000)

enum
{
    FAULT_EXIT_STATUS = 3
};

static uint32_t MmioRead32(uintptr_t address);
static void MmioWrite32(uintptr_t address, uint32_t value);

/* The driver sleeps between polls of a busy transmitter or an empty receiver,
 * so the other threads run while the console waits. */
static const CmsdkUartMemoryAccess MMIO_ACCESS = {MmioRead32, MmioWrite32, BddTargetOsPrimitives_Sleep};

void BddTargetBoard_Init(void)
{
    CmsdkUart_Init(&MMIO_ACCESS, CMSDK_UART0_BASE_ADDRESS);
}

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

void BddTargetBoard_ConsoleWrite(const char* text, size_t length)
{
    CmsdkUart_Write(text, length);
}

char BddTargetBoard_ConsoleRead(void)
{
    return CmsdkUart_GetChar();
}

/* The emulated transmitter is never busy, so the write does not sleep, and
 * can be made from wherever the fault was. The run then ends with a status of
 * its own, which no other exit gives. */
void BddTargetBoard_Fault(const char* what, const char* where)
{
    static const char START[] = "\r\n[board] fault: ";
    CmsdkUart_Write(START, strlen(START));
    CmsdkUart_Write(what, strlen(what));
    CmsdkUart_Write(" ", 1U);
    CmsdkUart_Write(where, strlen(where));
    CmsdkUart_Write("\r\n", 2U);
    BddTargetBoard_Exit(FAULT_EXIT_STATUS);
}

void BddTargetBoard_Exit(int status)
{
    /* SYS_EXIT_EXTENDED (0x20) - the only ARM Semihosting exit form on AArch32
     * that propagates a non-zero status: R1 points to a { reason, subcode }
     * block. QEMU terminates the VM; the for(;;) is defensive. */
    const struct
    {
        uint32_t reason;
        uint32_t subcode;
    } args = {0x20026U, (uint32_t) status};

    register int r0 __asm("r0") = 0x20;
    register const void* r1 __asm("r1") = &args;
    __asm volatile("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
    for (;;)
    {
    }
}
