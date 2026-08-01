#include "CppUTest/TestHarness.h"

extern "C"
{
#include "SolidSyslogBlockSequencePrivate.h"
#include "SolidSyslogTunables.h"
}

// clang-format off
TEST_GROUP(BlockSequencePool)
{
    /* SolidSyslogBlockSequence_Initialise only copies the config fields into the slot
     * (no dereferences); a zero-filled config is enough to exercise the
     * pool lifecycle without standing up a fake BlockDevice. */
    struct SolidSyslogBlockSequenceConfig config = {};
    struct SolidSyslogBlockSequence* pooled[SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE] = {};
    struct SolidSyslogBlockSequence* overflow                                   = nullptr;

    void teardown() override
    {
        for (auto*& slot : pooled)
        {
            SolidSyslogBlockSequence_Destroy(slot);
            slot = nullptr;
        }
        SolidSyslogBlockSequence_Destroy(overflow);
        overflow = nullptr;
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogBlockSequence_Create(&config);
        }
    }
};

// clang-format on

TEST(BlockSequencePool, CreateReturnsNonNullForFreshPool)
{
    struct SolidSyslogBlockSequence* slot = SolidSyslogBlockSequence_Create(&config);
    CHECK_TEXT(slot != nullptr, "first Create on empty pool returned NULL");
    SolidSyslogBlockSequence_Destroy(slot);
}

TEST(BlockSequencePool, FillingPoolThenOverflowReturnsNull)
{
    FillPool();

    overflow = SolidSyslogBlockSequence_Create(&config);

    CHECK_TEXT(overflow == nullptr, "exhausted pool should return NULL, not a handle");
    for (auto* slot : pooled)
    {
        CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");
    }
}

TEST(BlockSequencePool, DestroyReleasesSlotForReuse)
{
    FillPool();

    SolidSyslogBlockSequence_Destroy(pooled[0]);
    pooled[0] = SolidSyslogBlockSequence_Create(&config);

    CHECK_TEXT(pooled[0] != nullptr, "reacquire after Destroy returned NULL");
}

TEST(BlockSequencePool, DestroyOfNullIsSilentNoop)
{
    /* TU-internal classes return NULL on exhaustion (no shared null-object).
     * The only legitimate path to a NULL handle is a failed Create, and the
     * consumer's own error reporting covers that.
     * SolidSyslogBlockSequence_Destroy(NULL) must therefore be a silent
     * no-op. */
    SolidSyslogBlockSequence_Destroy(nullptr);
}
