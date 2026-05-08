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
