#include "CmsdkUart.h"

#include <stdbool.h>
#include <stddef.h>

#define DATA_OFFSET 0x000U
#define STATE_OFFSET 0x004U
#define CTRL_OFFSET 0x008U
#define BAUDDIV_OFFSET 0x010U

#define BAUD_DIVISOR 16U
#define TX_ENABLE 0x01U
#define TX_FULL_BIT 0x01U

#define YIELD_MILLISECONDS 1

static const CmsdkUartMemoryAccess* memoryAccess = NULL;
static uintptr_t                    base         = 0U;

static inline void SetBaudDivisor(void);
static inline void EnableTransmitter(void);
static inline bool TransmitterIsBusy(void);
static inline void Yield(void);
static inline void WriteDataRegister(char c);

void CmsdkUart_Init(const CmsdkUartMemoryAccess* access, uintptr_t baseAddress)
{
    memoryAccess = access;
    base         = baseAddress;
    SetBaudDivisor();
    EnableTransmitter();
}

static inline void SetBaudDivisor(void)
{
    memoryAccess->write32(base + BAUDDIV_OFFSET, BAUD_DIVISOR);
}

static inline void EnableTransmitter(void)
{
    memoryAccess->write32(base + CTRL_OFFSET, TX_ENABLE);
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
