#include <string>

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "LittleFsDisk.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogLittleFsFile.h"
#include "lfs.h"
}

/* What a write costs a flash part in erases, whatever state LittleFS's
 * directory was in as the file was opened.
 *
 * LittleFS decides whether a directory block still has erased space as it
 * opens a file. A file opened straight after a commit that ran to the end of
 * the block is told there is none, and as of v2.11.3 every commit through it
 * then erases a block for as long as it stays open. SolidSyslogLittleFsFile
 * opens the file again after each write, and the first test pins what that
 * buys.
 *
 * The second is its control: the same opens and the same writes, made on
 * LittleFS itself, where nothing opens the file again. Opening a file again
 * commits nothing, so the two leave the directory in the same state before
 * every open up to the first that follows such a commit, and meet that one
 * together. The control fails for one of two reasons, and either is worth
 * knowing: the opens below no longer reach that state, so the first test
 * proves nothing; or LittleFS no longer behaves this way, and the file need
 * not be opened again. The second is what a LittleFS carrying this fix brings:
 * https://github.com/littlefs-project/littlefs/pull/1248 */

enum
{
    /* The writes are of a different length before each open, so the
     * directory's last commit ends at a different place each time. A few of
     * these opens follow a commit that ran to the end of the block. */
    OPENS = 64,
    WRITES_AN_OPEN = 8,
    LONGEST_WRITE = 32,
    /* Now and then a write erases, as the directory block fills and LittleFS
     * compacts it. Half the writes doing so is beyond that. */
    MOST_ERASES_AN_OPEN = WRITES_AN_OPEN / 2
};

/* The length of the name matters: it sets where the directory's commits
 * fall. With this one, ten of the opens follow a commit that ran to the end
 * of the block, and a name a few characters either way does the same. */
static const char* const PATH = "/a-file-a-store-keeps-open.log";

static LittleFsDisk disk;
static lfs_config config;
static lfs_t filesystem;
static unsigned char fileCache[LITTLEFS_DISK_CACHE_SIZE];
static const unsigned char DATA[LONGEST_WRITE] = {};

// clang-format off
TEST_GROUP(SolidSyslogLittleFsFileWear)
{
    SolidSyslogFile* file = nullptr;

    void setup() override
    {
        LittleFsDisk_Init(&disk);
        LittleFsDisk_Configure(&disk, &config);
        LONGS_EQUAL(LFS_ERR_OK, lfs_format(&filesystem, &config));
        LONGS_EQUAL(LFS_ERR_OK, lfs_mount(&filesystem, &config));
        file = SolidSyslogLittleFsFile_Create(&filesystem, fileCache, sizeof(fileCache));
    }

    void teardown() override
    {
        SolidSyslogLittleFsFile_Destroy(file);
        (void) lfs_unmount(&filesystem);
    }

    /* Opens the file OPENS times and says what the writes of one open cost in
     * erases at the worst. `erasesForTheWritesOfOneOpen` opens it, writes
     * `length` bytes to it WRITES_AN_OPEN times, and closes it. */
    template <typename ErasesForTheWritesOfOneOpen>
    static unsigned WorstOfEveryOpen(ErasesForTheWritesOfOneOpen erasesForTheWritesOfOneOpen)
    {
        unsigned worst = 0;
        for (unsigned open = 0; open < static_cast<unsigned>(OPENS); open++)
        {
            const unsigned erases = erasesForTheWritesOfOneOpen(1U + (open % static_cast<unsigned>(LONGEST_WRITE)));
            worst = (erases > worst) ? erases : worst;
        }
        return worst;
    }

    [[nodiscard]] unsigned ErasesThroughTheFile(unsigned length) const
    {
        CHECK_TRUE(SolidSyslogFile_Open(file, PATH));
        const unsigned before = LittleFsDisk_EraseCount(&disk);
        for (unsigned write = 0; write < static_cast<unsigned>(WRITES_AN_OPEN); write++)
        {
            SolidSyslogFile_SeekTo(file, 0);
            CHECK_TRUE(SolidSyslogFile_Write(file, DATA, length));
        }
        const unsigned erases = LittleFsDisk_EraseCount(&disk) - before;
        SolidSyslogFile_Close(file);
        return erases;
    }

    /* What SolidSyslogLittleFsFile asks of LittleFS for the same open and the
     * same writes, less opening the file again. */
    static unsigned ErasesOnLittleFsItself(unsigned length)
    {
        lfs_file_t handle = {};
        lfs_file_config openConfig = {};
        openConfig.buffer = fileCache;
        LONGS_EQUAL(LFS_ERR_OK, lfs_file_opencfg(&filesystem, &handle, PATH, LFS_O_RDWR | LFS_O_CREAT, &openConfig));
        const unsigned before = LittleFsDisk_EraseCount(&disk);
        for (unsigned write = 0; write < static_cast<unsigned>(WRITES_AN_OPEN); write++)
        {
            LONGS_EQUAL(0, lfs_file_seek(&filesystem, &handle, 0, LFS_SEEK_SET));
            LONGS_EQUAL(length, lfs_file_write(&filesystem, &handle, DATA, length));
            LONGS_EQUAL(LFS_ERR_OK, lfs_file_sync(&filesystem, &handle));
        }
        const unsigned erases = LittleFsDisk_EraseCount(&disk) - before;
        LONGS_EQUAL(LFS_ERR_OK, lfs_file_close(&filesystem, &handle));
        return erases;
    }
};

// clang-format on

TEST(SolidSyslogLittleFsFileWear, TheWritesAfterAnyOpenCostFewErases)
{
    const unsigned worst = WorstOfEveryOpen([this](unsigned length) { return ErasesThroughTheFile(length); });

    CHECK_TEXT(
        worst <= static_cast<unsigned>(MOST_ERASES_AN_OPEN),
        (std::to_string(worst) + " erases for " + std::to_string(WRITES_AN_OPEN) + " writes").c_str()
    );
}

TEST(SolidSyslogLittleFsFileWear, AFileLeftOpenCanPayAnEraseForEveryWrite)
{
    const unsigned worst = WorstOfEveryOpen(ErasesOnLittleFsItself);

    CHECK_TEXT(
        worst >= static_cast<unsigned>(WRITES_AN_OPEN),
        (std::to_string(worst) + " erases for " + std::to_string(WRITES_AN_OPEN) + " writes").c_str()
    );
}
