#include <stddef.h>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogNullBuffer.h"

// clang-format off
TEST_GROUP(SolidSyslogNullBuffer)
{
    struct SolidSyslogBuffer* buffer = nullptr;

    void setup() override
    {
        buffer = SolidSyslogNullBuffer_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullBuffer, GetReturnsNonNull)
{
    CHECK_TRUE(buffer != nullptr);
}

TEST(SolidSyslogNullBuffer, WriteSwallowsRecord)
{
    SolidSyslogBuffer_Write(buffer, "hello", 5);
    // Doesn't crash; nothing to assert on the buffer state.
}

TEST(SolidSyslogNullBuffer, ReadReturnsFalseAndZeroBytes)
{
    char data[16] = {};
    size_t bytesRead = 99;
    CHECK_FALSE(SolidSyslogBuffer_Read(buffer, data, sizeof(data), &bytesRead));
    LONGS_EQUAL(0, bytesRead);
}

TEST(SolidSyslogNullBuffer, GetIsIdempotentAndReturnsSameInstance)
{
    POINTERS_EQUAL(buffer, SolidSyslogNullBuffer_Get());
}
