#include <glob.h>
#include <cstdio>
#include <cstring>
#include <string>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogPosixFile.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogTunables.h"

static const char* const TEST_PATH_PREFIX = "/tmp/test_posix_store";

static void CleanStoreFiles()
{
    glob_t results = {};
    std::string pattern = std::string(TEST_PATH_PREFIX) + "*.log";
    if (glob(pattern.c_str(), 0, nullptr, &results) == 0)
    {
        for (size_t i = 0; i < results.gl_pathc; i++)
        {
            std::remove(results.gl_pathv[i]);
        }
        globfree(&results);
    }
}

/* Integration tests using real POSIX files instead of FileFake.
 * These catch issues that only surface with real file handles —
 * e.g. reading from a deleted file, handle reuse after close,
 * or cursor state surviving across rotate/discard cycles. */

// clang-format off
TEST_GROUP(SolidSyslogBlockStorePosix)
{
    static const size_t RECORD_OVERHEAD    = 5; /* 2 (magic) + 2 (length) + 1 (sent flag) */
    static const size_t ONE_MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + RECORD_OVERHEAD;

    struct SolidSyslogFile* file = nullptr;
    struct SolidSyslogBlockDevice* device = nullptr;
    struct SolidSyslogStore* store = nullptr;
    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};

    void setup() override
    {
        CleanStoreFiles();
        file   = SolidSyslogPosixFile_Create();
        device = SolidSyslogFileBlockDevice_Create(file, TEST_PATH_PREFIX);
        std::memset(maxMsg, 'A', sizeof(maxMsg));
    }

    void teardown() override
    {
        SolidSyslogBlockStore_Destroy(store);
        SolidSyslogFileBlockDevice_Destroy(device);
        SolidSyslogPosixFile_Destroy(file);
        CleanStoreFiles();
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- test helper mirrors rotation test group signature
    void CreateStore(size_t maxBlockSize = ONE_MAX_MSG_RECORD, size_t maxBlocks = 2)
    {
        struct SolidSyslogBlockStoreConfig config = {};
        config.BlockDevice   = device;
        config.MaxBlockSize  = maxBlockSize;
        config.MaxBlocks     = maxBlocks;
        config.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = SolidSyslogBlockStore_Create(&config);
    }

    void WriteMaxMsg()
    {
        SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg));
    }
};

// clang-format on

TEST(SolidSyslogBlockStorePosix, DiscardOldestDrainYieldsOnlySurvivingRecords)
{
    CreateStore();

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    std::memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg)); /* file 00 — will be discarded */

    char secondMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    std::memset(secondMsg, 'C', sizeof(secondMsg));
    SolidSyslogStore_Write(store, secondMsg, sizeof(secondMsg)); /* file 01 — survives */

    WriteMaxMsg(); /* file 02 — triggers discard of file 00 */

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 0;

    /* First record should be from surviving file 01, not discarded file 00 */
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    BYTES_EQUAL('C', buf[0]);
    SolidSyslogStore_MarkSent(store);

    /* Second record from file 02 */
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    BYTES_EQUAL('A', buf[0]);
    SolidSyslogStore_MarkSent(store);

    /* No more records */
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStorePosix, DiscardOldestWhenReadIsPartwayThroughOldestFile)
{
    static const size_t TWO_MAX_MSG_RECORDS = 2 * ONE_MAX_MSG_RECORD;
    CreateStore(TWO_MAX_MSG_RECORDS);

    char msgA[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    char msgB[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    char msgC[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    char msgD[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    std::memset(msgA, 'A', sizeof(msgA));
    std::memset(msgB, 'B', sizeof(msgB));
    std::memset(msgC, 'C', sizeof(msgC));
    std::memset(msgD, 'D', sizeof(msgD));

    /* Fill file 00 with two records */
    SolidSyslogStore_Write(store, msgA, sizeof(msgA));
    SolidSyslogStore_Write(store, msgB, sizeof(msgB));

    /* Fill file 01 with two records */
    SolidSyslogStore_Write(store, msgC, sizeof(msgC));
    SolidSyslogStore_Write(store, msgD, sizeof(msgD));

    /* Read and send first record from file 00 — read cursor is now partway through */
    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 0;
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    BYTES_EQUAL('A', buf[0]);
    SolidSyslogStore_MarkSent(store);

    /* Write one more — triggers rotation to file 02 and discard of file 00 */
    WriteMaxMsg();

    /* Should get record B is lost (discarded with file 00), then C, D from file 01, then E from file 02 */
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    BYTES_EQUAL('C', buf[0]);
    SolidSyslogStore_MarkSent(store);

    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    BYTES_EQUAL('D', buf[0]);
    SolidSyslogStore_MarkSent(store);

    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    BYTES_EQUAL('A', buf[0]); /* maxMsg filled with 'A' */
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}
