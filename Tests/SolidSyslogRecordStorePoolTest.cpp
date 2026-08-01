#include "CppUTest/TestHarness.h"

extern "C"
{
#include "SolidSyslogRecordStorePrivate.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogTunables.h"
}

// clang-format off
TEST_GROUP(RecordStorePool)
{
    struct SolidSyslogSecurityPolicy* policy = nullptr;
    struct SolidSyslogRecordStore* pooled[SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE] = {};
    struct SolidSyslogRecordStore* overflow                                   = nullptr;

    void setup() override
    {
        policy = SolidSyslogNullSecurityPolicy_Get();
    }

    void teardown() override
    {
        for (auto*& slot : pooled)
        {
            SolidSyslogRecordStore_Destroy(slot);
            slot = nullptr;
        }
        SolidSyslogRecordStore_Destroy(overflow);
        overflow = nullptr;
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogRecordStore_Create(policy);
        }
    }
};

// clang-format on

TEST(RecordStorePool, CreateReturnsNonNullForFreshPool)
{
    struct SolidSyslogRecordStore* slot = SolidSyslogRecordStore_Create(policy);
    CHECK_TEXT(slot != nullptr, "first Create on empty pool returned NULL");
    SolidSyslogRecordStore_Destroy(slot);
}

TEST(RecordStorePool, FillingPoolThenOverflowReturnsNull)
{
    FillPool();

    overflow = SolidSyslogRecordStore_Create(policy);

    CHECK_TEXT(overflow == nullptr, "exhausted pool should return NULL, not a handle");
    for (auto* slot : pooled)
    {
        CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");
    }
}

TEST(RecordStorePool, DestroyReleasesSlotForReuse)
{
    FillPool();

    SolidSyslogRecordStore_Destroy(pooled[0]);
    pooled[0] = SolidSyslogRecordStore_Create(policy);

    CHECK_TEXT(pooled[0] != nullptr, "reacquire after Destroy returned NULL");
}

TEST(RecordStorePool, DestroyOfNullIsSilentNoop)
{
    /* TU-internal classes return NULL on exhaustion (no shared null-object).
     * The only legitimate path to a NULL handle is a failed Create, and the
     * consumer's own error reporting covers that.
     * SolidSyslogRecordStore_Destroy(NULL) must therefore be a silent
     * no-op. */
    SolidSyslogRecordStore_Destroy(nullptr);
}
