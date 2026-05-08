#include "CmsdkUart.h"

#include <stddef.h>

#define DATA_OFFSET 0x000U
#define STATE_OFFSET 0x004U
#define CTRL_OFFSET 0x008U
#define BAUDDIV_OFFSET 0x010U

#define BAUD_DIVISOR 16U
#define TX_ENABLE 0x01U
#define TX_FULL_BIT 0x01U

static const CmsdkUartMemoryAccess* memoryAccess = NULL;
static uintptr_t                    base         = 0U;

void CmsdkUart_Init(const CmsdkUartMemoryAccess* access, uintptr_t baseAddress)
{
    memoryAccess = access;
    base         = baseAddress;
    access->write32(baseAddress + BAUDDIV_OFFSET, BAUD_DIVISOR);
    access->write32(baseAddress + CTRL_OFFSET, TX_ENABLE);
}

void CmsdkUart_PutChar(char c)
{
    while ((memoryAccess->read32(base + STATE_OFFSET) & TX_FULL_BIT) != 0U)
    {
        /* Spin while the transmit holding register is occupied — writing to
         * DATA in this state sets STATE.TX_OVRE on silicon and the byte is
         * lost. QEMU's CMSDK model drains synchronously inside the DATA
         * write, so this spin never iterates there; it is silicon-correct
         * and the host-side fake exercises both the busy and idle paths. */
    }
    memoryAccess->write32(base + DATA_OFFSET, (uint32_t) (unsigned char) c);
}

void CmsdkUart_Write(const char* buffer, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        CmsdkUart_PutChar(buffer[i]);
    }
}
