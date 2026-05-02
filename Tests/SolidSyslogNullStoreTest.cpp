#include "CppUTest/TestHarness.h"
#include "SolidSyslogNullStore.h"

// clang-format off
TEST_GROUP(SolidSyslogNullStore)
{
    struct SolidSyslogStore* store = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = SolidSyslogNullStore_Create();
    }

    void teardown() override
    {
        SolidSyslogNullStore_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogNullStore, HasUnsentReturnsFalse)
{
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogNullStore, ReadNextUnsentReturnsFalse)
{
    char   data[512];
    size_t bytesRead = 0;
    CHECK_FALSE(SolidSyslogStore_ReadNextUnsent(store, data, sizeof(data), &bytesRead));
}

TEST(SolidSyslogNullStore, WriteReturnsTrue)
{
    CHECK_TRUE(SolidSyslogStore_Write(store, "hello", 5));
}

TEST(SolidSyslogNullStore, MarkSentDoesNotCrash)
{
    SolidSyslogStore_MarkSent(store);
}

TEST(SolidSyslogNullStore, GetTotalBytesReturnsZero)
{
    LONGS_EQUAL(0, SolidSyslogStore_GetTotalBytes(store));
}

TEST(SolidSyslogNullStore, GetUsedBytesReturnsZero)
{
    LONGS_EQUAL(0, SolidSyslogStore_GetUsedBytes(store));
}
