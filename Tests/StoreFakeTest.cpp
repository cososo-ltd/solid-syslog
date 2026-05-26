#include <stddef.h>

#include "StoreFake.h"
#include "SolidSyslogStore.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

static const char* const TEST_MESSAGE = "hello";
static const size_t TEST_MESSAGE_LEN = 5;

// clang-format off
TEST_GROUP(StoreFake)
{
    struct SolidSyslogStore* store = nullptr;
    char   readData[512];
    size_t readSize;

    void setup() override
    {
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

TEST(StoreFake, ReadsTwoWrittenMessagesInFifoOrderAcrossMarkSent)
{
    SolidSyslogStore_Write(store, "first", 5);
    SolidSyslogStore_Write(store, "second", 6);

    ReadNextUnsent();
    MEMCMP_EQUAL("first", readData, 5);
    LONGS_EQUAL(5, readSize);

    SolidSyslogStore_MarkSent(store);

    ReadNextUnsent();
    MEMCMP_EQUAL("second", readData, 6);
    LONGS_EQUAL(6, readSize);

    SolidSyslogStore_MarkSent(store);
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(StoreFake, WriteCountReportsSuccessfulWrites)
{
    SolidSyslogStore_Write(store, "a", 1);
    SolidSyslogStore_Write(store, "b", 1);
    CALLED_FAKE_ON(StoreFake_Write, store, TWICE);
}

TEST(StoreFake, WriteCountIgnoresFailedWrite)
{
    SolidSyslogStore_Write(store, "a", 1);
    StoreFake_FailNextWrite();
    SolidSyslogStore_Write(store, "b", 1);
    CALLED_FAKE_ON(StoreFake_Write, store, ONCE);
}
