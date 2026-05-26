#include "CmsdkUart.h"

#include <stdbool.h>
#include <stddef.h>

// The CMSDK UART register-offset, control-bit and status-bit macros below
// mirror the ARM CMSDK UART hardware datasheet exactly, plus the integer-
// valued YIELD_MILLISECONDS yield interval. The macro form is intentional —
// a future reader can grep `DATA_OFFSET` against vendor docs and land on
// these definitions verbatim. Converting them to enums would obscure that
// mapping, so we keep them as #defines. Mirrors the suppression in
// Tests/FreeRtos/CmsdkUartFake.c.
// NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum)
#define DATA_OFFSET 0x000U
#define STATE_OFFSET 0x004U
#define CTRL_OFFSET 0x008U
#define BAUDDIV_OFFSET 0x010U

#define BAUD_DIVISOR 16U
#define TX_ENABLE 0x01U
#define RX_ENABLE 0x02U
#define TX_FULL_BIT 0x01U
#define RX_FULL_BIT 0x02U

#define YIELD_MILLISECONDS 1
// NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum)

static const CmsdkUartMemoryAccess* memoryAccess = NULL;
static uintptr_t base = 0U;

static inline void SetBaudDivisor(void);
static inline void EnableTxAndRx(void);
static inline bool TransmitterIsBusy(void);
static inline void Yield(void);
static inline void WriteDataRegister(char c);
static inline bool ReceiverHasByte(void);
static inline char ReadDataRegister(void);

void CmsdkUart_Init(const CmsdkUartMemoryAccess* access, uintptr_t baseAddress)
{
    memoryAccess = access;
    base = baseAddress;
    SetBaudDivisor();
    EnableTxAndRx();
}

static inline void SetBaudDivisor(void)
{
    memoryAccess->write32(base + BAUDDIV_OFFSET, BAUD_DIVISOR);
}

static inline void EnableTxAndRx(void)
{
    memoryAccess->write32(base + CTRL_OFFSET, TX_ENABLE | RX_ENABLE);
}

void CmsdkUart_PutChar(char c)
{
    while (TransmitterIsBusy())
    {
        Yield();
    }
    WriteDataRegister(c);
}

static inline bool TransmitterIsBusy(void)
{
    return (memoryAccess->read32(base + STATE_OFFSET) & TX_FULL_BIT) != 0U;
}

static inline void Yield(void)
{
    memoryAccess->sleep(YIELD_MILLISECONDS);
}

static inline void WriteDataRegister(char c)
{
    memoryAccess->write32(base + DATA_OFFSET, (uint32_t) (unsigned char) c);
}

void CmsdkUart_Write(const char* buffer, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        CmsdkUart_PutChar(buffer[i]);
    }
}

char CmsdkUart_GetChar(void)
{
    while (!ReceiverHasByte())
    {
        Yield();
    }
    return ReadDataRegister();
}

static inline bool ReceiverHasByte(void)
{
    return (memoryAccess->read32(base + STATE_OFFSET) & RX_FULL_BIT) != 0U;
}

static inline char ReadDataRegister(void)
{
    return (char) (memoryAccess->read32(base + DATA_OFFSET) & 0xFFU);
}
