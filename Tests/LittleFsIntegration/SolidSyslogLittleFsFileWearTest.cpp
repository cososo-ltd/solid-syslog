#include <string>

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "LittleFsDisk.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogLittleFsFile.h"
#include "SolidSyslogTunables.h"
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
 * The second is its control. A path too long for the file to keep is not
 * opened again, so it shows what LittleFS does left alone. It fails for one of
 * two reasons, and either is worth knowing: the opens below no longer reach
 * that state, so the first test proves nothing; or LittleFS no longer behaves
 * this way, and the file need not be opened again. The second is what a
 * LittleFS carrying this fix brings:
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

static LittleFsDisk disk;
static lfs_config config;
static lfs_t filesystem;
static unsigned char fileCache[LITTLEFS_DISK_CACHE_SIZE];

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

    /* Opens the file OPENS times, writing to it a few times each, and says
     * what the writes of one open cost in erases at the worst. */
    [[nodiscard]] unsigned WorstErasesForTheWritesOfOneOpen(const std::string& path) const
    {
        const unsigned char data[LONGEST_WRITE] = {};
        unsigned worst = 0;
        for (unsigned open = 0; open < static_cast<unsigned>(OPENS); open++)
        {
            CHECK_TRUE(SolidSyslogFile_Open(file, path.c_str()));
            const unsigned before = LittleFsDisk_EraseCount(&disk);
            for (unsigned write = 0; write < static_cast<unsigned>(WRITES_AN_OPEN); write++)
            {
                SolidSyslogFile_SeekTo(file, 0);
                CHECK_TRUE(SolidSyslogFile_Write(file, data, 1U + (open % static_cast<unsigned>(LONGEST_WRITE))));
            }
            const unsigned erases = LittleFsDisk_EraseCount(&disk) - before;
            worst = (erases > worst) ? erases : worst;
            SolidSyslogFile_Close(file);
        }
        return worst;
    }
};

// clang-format on

TEST(SolidSyslogLittleFsFileWear, TheWritesAfterAnyOpenCostFewErases)
{
    const std::string pathThatIsKept(SOLIDSYSLOG_MAX_PATH_SIZE - 1U, 'p');

    const unsigned worst = WorstErasesForTheWritesOfOneOpen(pathThatIsKept);

    CHECK_TEXT(
        worst <= static_cast<unsigned>(MOST_ERASES_AN_OPEN),
        (std::to_string(worst) + " erases for " + std::to_string(WRITES_AN_OPEN) + " writes").c_str()
    );
}

TEST(SolidSyslogLittleFsFileWear, AFileLeftOpenCanPayAnEraseForEveryWrite)
{
    const std::string pathTooLongToKeep(SOLIDSYSLOG_MAX_PATH_SIZE, 'p');

    const unsigned worst = WorstErasesForTheWritesOfOneOpen(pathTooLongToKeep);

    CHECK_TEXT(
        worst >= static_cast<unsigned>(WRITES_AN_OPEN),
        (std::to_string(worst) + " erases for " + std::to_string(WRITES_AN_OPEN) + " writes").c_str()
    );
}
