#include "CppUTest/TestHarness.h"

extern "C"
{
#include "BlockSequencePrivate.h"
#include "SolidSyslogTunables.h"
}

// clang-format off
TEST_GROUP(BlockSequencePool)
{
    /* BlockSequence_Initialise only copies the config fields into the slot
     * (no dereferences); a zero-filled config is enough to exercise the
     * pool lifecycle without standing up a fake BlockDevice. */
    struct BlockSequenceConfig config = {};
    struct BlockSequence* pooled[SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE] = {};
    struct BlockSequence* overflow                                   = nullptr;

    void teardown() override
    {
        for (auto*& slot : pooled)
        {
            BlockSequence_Destroy(slot);
            slot = nullptr;
        }
        BlockSequence_Destroy(overflow);
        overflow = nullptr;
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = BlockSequence_Create(&config);
        }
    }
};

// clang-format on

TEST(BlockSequencePool, CreateReturnsNonNullForFreshPool)
{
    struct BlockSequence* slot = BlockSequence_Create(&config);
    CHECK_TEXT(slot != nullptr, "first Create on empty pool returned NULL");
    BlockSequence_Destroy(slot);
}

TEST(BlockSequencePool, FillingPoolThenOverflowReturnsNull)
{
    FillPool();

    overflow = BlockSequence_Create(&config);

    CHECK_TEXT(overflow == nullptr, "exhausted pool should return NULL, not a handle");
    for (auto* slot : pooled)
    {
        CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");
    }
}

TEST(BlockSequencePool, DestroyReleasesSlotForReuse)
{
    FillPool();

    BlockSequence_Destroy(pooled[0]);
    pooled[0] = BlockSequence_Create(&config);

    CHECK_TEXT(pooled[0] != nullptr, "reacquire after Destroy returned NULL");
}

TEST(BlockSequencePool, DestroyOfNullIsSilentNoop)
{
    /* TU-internal classes return NULL on exhaustion (no shared null-object).
     * The only legitimate path to a NULL handle is a failed Create, and the
     * consumer's own error reporting covers that. _Destroy(NULL) must therefore
     * be a silent no-op. */
    BlockSequence_Destroy(nullptr);
}
