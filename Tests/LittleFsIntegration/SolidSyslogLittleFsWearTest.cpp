#include <cstring>
#include <string>

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "LittleFsDisk.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogLittleFsFile.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogStoreDefinition.h"
#include "lfs.h"
}

/* What the store costs a flash part, in erases, which are what wear one out.
 * The platform page tells an integrator how the size of a store file sets
 * that cost; these hold it to what LittleFS does.
 *
 * LittleFS never programs a block again once a commit refers to it. So an
 * append to a file copies the file's last block to a new one, and a write in
 * the middle of a file - the store's mark that a record was sent - copies the
 * block it is in and every block after it. Each block copied to is erased
 * first.
 *
 * The counts take in what LittleFS spends on its own metadata as well, which
 * it compacts every so many commits, so most are held between bounds. */

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

namespace
{
struct WearStack
{
    LittleFsDisk disk = {};
    lfs_config config = {};
    lfs_t filesystem = {};
    unsigned char fileCache[LITTLEFS_DISK_CACHE_SIZE] = {};
    SolidSyslogFile* file = nullptr;
    SolidSyslogBlockDevice* device = nullptr;
    SolidSyslogStore* store = nullptr;

    void Start(size_t storeFileBytes)
    {
        LittleFsDisk_Init(&disk);
        LittleFsDisk_Configure(&disk, &config);
        LONGS_EQUAL(LFS_ERR_OK, lfs_format(&filesystem, &config));
        LONGS_EQUAL(LFS_ERR_OK, lfs_mount(&filesystem, &config));
        file = SolidSyslogLittleFsFile_Create(&filesystem, fileCache, sizeof(fileCache));
        device = SolidSyslogFileBlockDevice_Create(file, "/STORE", storeFileBytes);
        SolidSyslogBlockStoreConfig storeConfig = {};
        storeConfig.BlockDevice = device;
        storeConfig.MaxBlocks = 32;
        store = SolidSyslogBlockStore_Create(&storeConfig);
    }

    void Stop()
    {
        SolidSyslogBlockStore_Destroy(store);
        SolidSyslogFileBlockDevice_Destroy(device);
        SolidSyslogLittleFsFile_Destroy(file);
        (void) lfs_unmount(&filesystem);
    }
};
} // namespace

static WearStack stack;

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

// clang-format off
TEST_GROUP(SolidSyslogLittleFsWear)
{
    void teardown() override
    {
        stack.Stop();
    }
};

// clang-format on

TEST(SolidSyslogLittleFsWear, AnAppendCostsAboutOneErase)
{
    stack.Start(FOUR_BLOCK_FILE_BYTES);
    const unsigned before = Erases();

    Append(RECORDS, RECORD_BYTES);

    CHECK_ERASES_A_RECORD(9U, 15U, Erases() - before, static_cast<unsigned>(RECORDS));
}

TEST(SolidSyslogLittleFsWear, AMarkInAFileOfOneBlockCostsAboutOneErase)
{
    stack.Start(ONE_BLOCK_FILE_BYTES);
    Append(RECORDS, RECORD_BYTES);
    const unsigned before = Erases();

    LONGS_EQUAL(RECORDS, Send(RECORDS));

    CHECK_ERASES_A_RECORD(9U, 15U, Erases() - before, static_cast<unsigned>(RECORDS));
}

TEST(SolidSyslogLittleFsWear, AMarkAtTheStartOfAFileErasesABlockForEveryBlockOfTheFile)
{
    stack.Start(FOUR_BLOCK_FILE_BYTES);
    Append(RECORDS, RECORD_BYTES);
    const unsigned before = Erases();

    LONGS_EQUAL(1, Send(1));

    LONGS_EQUAL(BLOCKS_OF_RECORDS, Erases() - before);
}

TEST(SolidSyslogLittleFsWear, AMarkAtTheEndOfAFileErasesOneBlock)
{
    stack.Start(FOUR_BLOCK_FILE_BYTES);
    Append(RECORDS - 1, RECORD_BYTES);
    LONGS_EQUAL(RECORDS - 1, Send(RECORDS - 1));
    Append(1, RECORD_BYTES);
    const unsigned before = Erases();

    LONGS_EQUAL(1, Send(1));

    LONGS_EQUAL(1, Erases() - before);
}

TEST(SolidSyslogLittleFsWear, ARecordStoredAndSentAtOnceCostsAboutTwoErases)
{
    stack.Start(FOUR_BLOCK_FILE_BYTES);
    const unsigned before = Erases();

    for (unsigned i = 0; i < static_cast<unsigned>(RECORDS); i++)
    {
        Append(1, RECORD_BYTES);
        LONGS_EQUAL(1, Send(1));
    }

    CHECK_ERASES_A_RECORD(18U, 25U, Erases() - before, static_cast<unsigned>(RECORDS));
}
