#include "CmsdkUartFake.h"

#include <stddef.h>

#define DATA_OFFSET 0x000U
#define STATE_OFFSET 0x004U
#define CTRL_OFFSET 0x008U
#define INTSTAT_OFFSET 0x00CU
#define BAUDDIV_OFFSET 0x010U

#define TX_FULL_BIT 0x01U
#define RX_FULL_BIT 0x02U
#define TX_OVRE_BIT 0x04U

static struct
{
    uintptr_t base;
    uint32_t  data;
    uint32_t  state;
    uint32_t  ctrl;
    uint32_t  intStatusOrClear;
    uint32_t  bauddiv;
    int       readsBeforeTxReadyDefault;
    int       readsRemainingBeforeTxReady;
    bool      txOverrunOccurred;
    int       sleepCallCount;
    uint32_t  receivedByte;
    int       readsBeforeRxReadyDefault;
    int       readsRemainingBeforeRxReady;
} fake;

static uint32_t Fake_Read32(uintptr_t address)
{
    uint32_t  result = 0;
    uintptr_t offset = address - fake.base;
    if (offset == STATE_OFFSET)
    {
        if (fake.readsRemainingBeforeTxReady > 0)
        {
            fake.readsRemainingBeforeTxReady--;
        }
        if (fake.readsRemainingBeforeTxReady == 0)
        {
            fake.state &= ~TX_FULL_BIT;
        }
        if (fake.readsRemainingBeforeRxReady > 0)
        {
            fake.readsRemainingBeforeRxReady--;
            if (fake.readsRemainingBeforeRxReady == 0)
            {
                fake.state |= RX_FULL_BIT;
            }
        }
        result = fake.state;
    }
    else if (offset == DATA_OFFSET)
    {
        if ((fake.state & RX_FULL_BIT) != 0U)
        {
            result = fake.receivedByte;
            fake.state &= ~RX_FULL_BIT;
        }
    }
    else if (offset == CTRL_OFFSET)
    {
        result = fake.ctrl;
    }
    else if (offset == BAUDDIV_OFFSET)
    {
        result = fake.bauddiv;
    }
    else if (offset == INTSTAT_OFFSET)
    {
        result = fake.intStatusOrClear;
    }
    return result;
}

static void Fake_Write32(uintptr_t address, uint32_t value)
{
    uintptr_t offset = address - fake.base;
    if (offset == DATA_OFFSET)
    {
        if ((fake.state & TX_FULL_BIT) != 0U)
        {
            fake.state |= TX_OVRE_BIT;
            fake.txOverrunOccurred = true;
        }
        fake.data = value;
        fake.state |= TX_FULL_BIT;
        fake.readsRemainingBeforeTxReady = fake.readsBeforeTxReadyDefault;
    }
    else if (offset == STATE_OFFSET)
    {
        /* W1C — only the overrun bits are software-clearable. */
        fake.state &= ~(value & (TX_OVRE_BIT | 0x08U));
    }
    else if (offset == CTRL_OFFSET)
    {
        fake.ctrl = value;
    }
    else if (offset == BAUDDIV_OFFSET)
    {
        fake.bauddiv = value;
    }
    else if (offset == INTSTAT_OFFSET)
    {
        fake.state &= ~(value & (TX_OVRE_BIT | 0x08U));
        fake.intStatusOrClear &= ~value;
    }
}

static void Fake_Sleep(int milliseconds)
{
    (void) milliseconds;
    fake.sleepCallCount++;
}

static const CmsdkUartMemoryAccess FAKE_ACCESS = {Fake_Read32, Fake_Write32, Fake_Sleep};

void CmsdkUartFake_Reset(uintptr_t baseAddress)
{
    fake.base                        = baseAddress;
    fake.data                        = 0U;
    fake.state                       = 0U;
    fake.ctrl                        = 0U;
    fake.intStatusOrClear            = 0U;
    fake.bauddiv                     = 0U;
    fake.readsBeforeTxReadyDefault   = 2;
    fake.readsRemainingBeforeTxReady = 0;
    fake.txOverrunOccurred           = false;
    fake.sleepCallCount              = 0;
    fake.receivedByte                = 0U;
    fake.readsBeforeRxReadyDefault   = 0;
    fake.readsRemainingBeforeRxReady = 0;
}

const CmsdkUartMemoryAccess* CmsdkUartFake_Access(void)
{
    return &FAKE_ACCESS;
}

uint32_t CmsdkUartFake_GetData(void)
{
    return fake.data;
}

uint32_t CmsdkUartFake_GetCtrl(void)
{
    return fake.ctrl;
}

uint32_t CmsdkUartFake_GetBaudDiv(void)
{
    return fake.bauddiv;
}

void CmsdkUartFake_SetReadsBeforeTxReady(int reads)
{
    fake.readsBeforeTxReadyDefault = reads;
}

bool CmsdkUartFake_TxOverrunOccurred(void)
{
    return fake.txOverrunOccurred;
}

int CmsdkUartFake_SleepCallCount(void)
{
    return fake.sleepCallCount;
}

void CmsdkUartFake_SetReceivedByte(char byte)
{
    /* Clear leftover RX-ready state from a prior arm so back-to-back calls
     * don't carry RX_FULL_BIT or a stale countdown into the new mode. */
    fake.receivedByte = (uint32_t) (unsigned char) byte;
    fake.state &= ~RX_FULL_BIT;
    fake.readsRemainingBeforeRxReady = 0;
    if (fake.readsBeforeRxReadyDefault == 0)
    {
        fake.state |= RX_FULL_BIT;
    }
    else
    {
        fake.readsRemainingBeforeRxReady = fake.readsBeforeRxReadyDefault;
    }
}

void CmsdkUartFake_SetReadsBeforeRxReady(int reads)
{
    fake.readsBeforeRxReadyDefault = reads;
}
