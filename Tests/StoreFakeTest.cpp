#include <stddef.h>

#include "StoreFake.h"
#include "SolidSyslogStore.h"
#include "CppUTest/TestHarness.h"

static const char* const TEST_MESSAGE     = "hello";
static const size_t      TEST_MESSAGE_LEN = 5;

// clang-format off
TEST_GROUP(StoreFake)
{
    struct SolidSyslogStore* store = nullptr;
    char   readData[512];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    size_t readSize;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = StoreFake_Create();
        readSize = 0;
    }

    void teardown() override
    {
        StoreFake_Destroy();
    }

    void Write() const
    {
        SolidSyslogStore_Write(store, TEST_MESSAGE, TEST_MESSAGE_LEN);
    }

    bool ReadNextUnsent()
    {
        return SolidSyslogStore_ReadNextUnsent(store, readData, sizeof(readData), &readSize);
    }
};

// clang-format on

TEST(StoreFake, HasUnsentReturnsFalseWhenEmpty)
{
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(StoreFake, WriteAndHasUnsentReturnsTrue)
{
    Write();
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
}

TEST(StoreFake, ReadNextUnsentReturnsWrittenData)
{
    Write();
    ReadNextUnsent();
    MEMCMP_EQUAL(TEST_MESSAGE, readData, TEST_MESSAGE_LEN);
}

TEST(StoreFake, ReadNextUnsentReturnsWrittenSize)
{
    Write();
    ReadNextUnsent();
    LONGS_EQUAL(TEST_MESSAGE_LEN, readSize);
}

TEST(StoreFake, MarkSentClearsUnsent)
{
    Write();
    SolidSyslogStore_MarkSent(store);
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(StoreFake, WriteCanBeConfiguredToFail)
{
    StoreFake_FailNextWrite();
    CHECK_FALSE(SolidSyslogStore_Write(store, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(StoreFake, FailedWriteDoesNotSetUnsent)
{
    StoreFake_FailNextWrite();
    SolidSyslogStore_Write(store, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(StoreFake, ReadCanBeConfiguredToFail)
{
    Write();
    StoreFake_FailNextRead();
    CHECK_FALSE(ReadNextUnsent());
}
