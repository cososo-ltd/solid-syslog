#include <stddef.h>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogStore.h"

// clang-format off
TEST_GROUP(SolidSyslogNullStore)
{
    struct SolidSyslogStore* store = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = SolidSyslogNullStore_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullStore, HasUnsentReturnsFalse)
{
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogNullStore, ReadNextUnsentReturnsFalse)
{
    char data[512];
    size_t bytesRead = 0;
    CHECK_FALSE(SolidSyslogStore_ReadNextUnsent(store, data, sizeof(data), &bytesRead));
}

TEST(SolidSyslogNullStore, WriteReturnsFalseToSignalNotRetained)
{
    /* The Store_Write contract reads "true = retained for later replay; false
     * = not held". NullStore never retains, so reports false — the eager-drain
     * loop in ProcessMessages then takes the direct-send fallback, preserving
     * the constrained-system "one attempt per message, no buffering" path. */
    CHECK_FALSE(SolidSyslogStore_Write(store, "hello", 5));
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
