#include "CmsdkUart.h"

#include <stdbool.h>
#include <stddef.h>

// The CMSDK UART register-offset, control-bit and status-bit values below
// mirror the ARM CMSDK UART hardware datasheet exactly, plus the integer-
// valued YIELD_MILLISECONDS yield interval. Grouped into an anonymous enum
// so a future reader can grep `DATA_OFFSET` against vendor docs and land on
// these definitions verbatim, while gaining a typed identifier over the
// historical #define form. Mirrors the values in Tests/FreeRtos/CmsdkUartFake.c.
enum
{
    DATA_OFFSET = 0x000,
    STATE_OFFSET = 0x004,
    CTRL_OFFSET = 0x008,
    BAUDDIV_OFFSET = 0x010,

    BAUD_DIVISOR = 16,
    TX_ENABLE = 0x01,
    RX_ENABLE = 0x02,
    TX_FULL_BIT = 0x01,
    RX_FULL_BIT = 0x02,

    YIELD_MILLISECONDS = 1,
};

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
