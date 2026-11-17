#include <cstring>
#include <string>

#include "CppUTest/TestHarness.h"

#include "LittleFsStoreStack.h"

/* What the store costs a flash part in erases. These tests pin the figures
 * the LittleFS platform page gives an integrator.
 *
 * LittleFS never programs a block again once a commit refers to it. An append
 * copies the file's last block to a new one, and the store's mark that a
 * record was sent, written in place, copies the block it is in and every
 * block after it. Each block copied to is erased first.
 *
 * The counts include what LittleFS spends compacting its own metadata, so
 * each is held between bounds. */

enum
{
    RECORD_BYTES = 40,
    RECORDS = 40,
    /* A store file of one filesystem block, and one of several. */
    ONE_BLOCK_FILE_BYTES = LITTLEFS_DISK_BLOCK_SIZE,
    FOUR_BLOCK_FILE_BYTES = 4 * LITTLEFS_DISK_BLOCK_SIZE,
    /* What RECORDS records come to in a file large enough to hold them all. */
    BLOCKS_OF_RECORDS = 4
};

static LittleFsStoreStack stack;

static unsigned Erases()
{
    return LittleFsDisk_EraseCount(&stack.disk);
}

static void Append(unsigned count, size_t bytes)
{
    const std::string record(bytes, 'r');
    for (unsigned i = 0; i < count; i++)
    {
        LONGS_EQUAL(SOLIDSYSLOG_STORE_WRITE_KEPT, SolidSyslogStore_Write(stack.store, record.data(), record.size()));
    }
}

static unsigned Send(unsigned count)
{
    char buffer[256] = {};
    size_t bytesRead = 0U;
    unsigned sent = 0;
    while ((sent < count) && SolidSyslogStore_ReadNextUnsent(stack.store, buffer, sizeof(buffer), &bytesRead) &&
           (bytesRead > 0U))
    {
        SolidSyslogStore_MarkSent(stack.store);
        sent++;
        bytesRead = 0;
    }
    return sent;
}

/* `erases` for `count` records came to between `least` and `most`, which are
 * in tenths of an erase a record. A macro, so a failure is reported at the
 * test's own line. */
#define CHECK_ERASES_A_RECORD(least, most, erases, count)                                      \
    CHECK_TEXT(                                                                                \
        (((erases) * 10U) >= ((count) * (least))) && (((erases) * 10U) <= ((count) * (most))), \
        (std::to_string(erases) + " erases for " + std::to_string(count) + " records").c_str() \
    )

/* One operation cost `blocks` erases, or one more where LittleFS compacted its
 * metadata under it. */
#define CHECK_ERASES(blocks, erases)                                                                       \
    CHECK_TEXT(                                                                                            \
        ((erases) >= static_cast<unsigned>(blocks)) && ((erases) <= (static_cast<unsigned>(blocks) + 1U)), \
        (std::to_string(erases) + " erases").c_str()                                                       \
    )

// clang-format off
TEST_GROUP(SolidSyslogLittleFsWear)
{
    void setup() override
    {
        stack = LittleFsStoreStack();
        stack.storeMaxBlocks = 32;
    }

    void teardown() override
    {
        stack.Unmount();
    }

    static void StartWithStoreBlocksOf(size_t bytes)
    {
        stack.storeBlockBytes = bytes;
        stack.FormatAndMount();
    }
};

// clang-format on

TEST(SolidSyslogLittleFsWear, AnAppendCostsAboutOneErase)
{
    StartWithStoreBlocksOf(FOUR_BLOCK_FILE_BYTES);
    const unsigned before = Erases();

    Append(RECORDS, RECORD_BYTES);

    CHECK_ERASES_A_RECORD(9U, 15U, Erases() - before, static_cast<unsigned>(RECORDS));
}

TEST(SolidSyslogLittleFsWear, AMarkInAFileOfOneBlockCostsAboutOneErase)
{
    StartWithStoreBlocksOf(ONE_BLOCK_FILE_BYTES);
    Append(RECORDS, RECORD_BYTES);
    const unsigned before = Erases();

    LONGS_EQUAL(RECORDS, Send(RECORDS));

    CHECK_ERASES_A_RECORD(9U, 15U, Erases() - before, static_cast<unsigned>(RECORDS));
}

TEST(SolidSyslogLittleFsWear, AMarkAtTheStartOfAFileErasesABlockForEveryBlockOfTheFile)
{
    StartWithStoreBlocksOf(FOUR_BLOCK_FILE_BYTES);
    Append(RECORDS, RECORD_BYTES);
    const unsigned before = Erases();

    LONGS_EQUAL(1, Send(1));

    CHECK_ERASES(BLOCKS_OF_RECORDS, Erases() - before);
}

TEST(SolidSyslogLittleFsWear, AMarkAtTheEndOfAFileErasesOneBlock)
{
    StartWithStoreBlocksOf(FOUR_BLOCK_FILE_BYTES);
    Append(RECORDS - 1, RECORD_BYTES);
    LONGS_EQUAL(RECORDS - 1, Send(RECORDS - 1));
    Append(1, RECORD_BYTES);
    const unsigned before = Erases();

    LONGS_EQUAL(1, Send(1));

    CHECK_ERASES(1U, Erases() - before);
}

TEST(SolidSyslogLittleFsWear, ARecordStoredAndSentAtOnceCostsAboutTwoErases)
{
    StartWithStoreBlocksOf(FOUR_BLOCK_FILE_BYTES);
    const unsigned before = Erases();

    for (unsigned i = 0; i < static_cast<unsigned>(RECORDS); i++)
    {
        Append(1, RECORD_BYTES);
        LONGS_EQUAL(1, Send(1));
    }

    CHECK_ERASES_A_RECORD(18U, 25U, Erases() - before, static_cast<unsigned>(RECORDS));
}
