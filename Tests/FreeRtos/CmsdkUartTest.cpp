#include "CppUTest/TestHarness.h"

#include "CmsdkUart.h"
#include "CmsdkUartFake.h"

static const uintptr_t TEST_BASE_ADDRESS = 0x40004000U;

// clang-format off
TEST_GROUP(CmsdkUart)
{
    void setup() override
    {
        CmsdkUartFake_Reset(TEST_BASE_ADDRESS);
        CmsdkUart_Init(CmsdkUartFake_Access(), TEST_BASE_ADDRESS);
    }
};
// clang-format on

TEST(CmsdkUart, InitWritesBaudDivisor)
{
    LONGS_EQUAL(16, CmsdkUartFake_GetBaudDiv());
}

TEST(CmsdkUart, InitEnablesTransmitter)
{
    LONGS_EQUAL(0x01, CmsdkUartFake_GetCtrl() & 0x01);
}

TEST(CmsdkUart, InitEnablesReceiver)
{
    LONGS_EQUAL(0x02, CmsdkUartFake_GetCtrl() & 0x02);
}

TEST(CmsdkUart, PutCharWritesByteToDataRegister)
{
    CmsdkUart_PutChar('A');
    LONGS_EQUAL('A', CmsdkUartFake_GetData());
}

TEST(CmsdkUart, PutCharWritesTheGivenByte)
{
    CmsdkUart_PutChar('B');
    LONGS_EQUAL('B', CmsdkUartFake_GetData());
}

TEST(CmsdkUart, PutCharSpinsForTxFullToClearBeforeWritingNextByte)
{
    CmsdkUart_PutChar('A');
    CmsdkUart_PutChar('B');
    LONGS_EQUAL('B', CmsdkUartFake_GetData());
    CHECK_FALSE(CmsdkUartFake_TxOverrunOccurred());
}

TEST(CmsdkUart, PutCharCallsSleepWhileSpinningForTxFull)
{
    CmsdkUart_PutChar('A');
    CmsdkUart_PutChar('B');
    CHECK(CmsdkUartFake_SleepCallCount() > 0);
}

TEST(CmsdkUart, PutCharWritesImmediatelyWhenTransmitterIsAlwaysReady)
{
    CmsdkUartFake_SetReadsBeforeTxReady(0);
    CmsdkUart_PutChar('X');
    CmsdkUart_PutChar('Y');
    LONGS_EQUAL('Y', CmsdkUartFake_GetData());
    CHECK_FALSE(CmsdkUartFake_TxOverrunOccurred());
}

TEST(CmsdkUart, WriteOfSingleByteEmitsThatByte)
{
    CmsdkUart_Write("X", 1);
    LONGS_EQUAL('X', CmsdkUartFake_GetData());
}

TEST(CmsdkUart, WriteOfMultipleBytesEmitsAllByteWithoutOverrun)
{
    CmsdkUart_Write("AB", 2);
    LONGS_EQUAL('B', CmsdkUartFake_GetData());
    CHECK_FALSE(CmsdkUartFake_TxOverrunOccurred());
}

TEST(CmsdkUart, GetCharReturnsByteFromDataRegister)
{
    CmsdkUartFake_SetReceivedByte('Q');
    LONGS_EQUAL('Q', CmsdkUart_GetChar());
}

TEST(CmsdkUart, GetCharSpinsForRxFullToBecomeSetBeforeReadingDataRegister)
{
    CmsdkUartFake_SetReadsBeforeRxReady(2);
    CmsdkUartFake_SetReceivedByte('Z');
    LONGS_EQUAL('Z', CmsdkUart_GetChar());
}

TEST(CmsdkUart, GetCharCallsSleepWhileSpinningForRxFull)
{
    CmsdkUartFake_SetReadsBeforeRxReady(2);
    CmsdkUartFake_SetReceivedByte('Z');
    (void) CmsdkUart_GetChar();
    CHECK(CmsdkUartFake_SleepCallCount() > 0);
}

TEST(CmsdkUart, GetCharReturnsImmediatelyWhenReceiverHasByte)
{
    CmsdkUartFake_SetReadsBeforeRxReady(0);
    CmsdkUartFake_SetReceivedByte('X');
    LONGS_EQUAL('X', CmsdkUart_GetChar());
    LONGS_EQUAL(0, CmsdkUartFake_SleepCallCount());
}

TEST(CmsdkUart, GetCharSpinsAfterReArmFromImmediateReadyToDelayedReady)
{
    CmsdkUartFake_SetReadsBeforeRxReady(0);
    CmsdkUartFake_SetReceivedByte('A');
    CmsdkUartFake_SetReadsBeforeRxReady(2);
    CmsdkUartFake_SetReceivedByte('B');
    LONGS_EQUAL('B', CmsdkUart_GetChar());
    CHECK(CmsdkUartFake_SleepCallCount() > 0);
}
