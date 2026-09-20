#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

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
#include "lfs.h"
}

/* The claim under test is the File contract's: a true return from Write means
 * the bytes are on the media, and the BlockStore treats that as surviving a
 * power loss. The unit suite proves the adapter calls lfs_file_sync against a
 * fake; only real LittleFS over a device that can lose power proves the bytes
 * are still there afterwards. */

static const char* const STORE_PREFIX = "/STORE";
static const size_t STORE_BLOCK_BYTES = 1024;
static const size_t STORE_MAX_BLOCKS = 8;

namespace
{

/* The whole stack over one emulated flash part. Mount and Unmount bracket the
 * library state, so a test can throw all of it away and come back to the same
 * bytes - which is what a reboot does. */
struct Stack
{
    LittleFsDisk disk = {};
    lfs_config config = {};
    lfs_t filesystem = {};
    unsigned char fileCache[LITTLEFS_DISK_CACHE_SIZE] = {};
    SolidSyslogFile* file = nullptr;
    SolidSyslogBlockDevice* device = nullptr;
    SolidSyslogStore* store = nullptr;

    void FormatAndMount()
    {
        LittleFsDisk_Init(&disk);
        LittleFsDisk_Configure(&disk, &config);
        LONGS_EQUAL(LFS_ERR_OK, lfs_format(&filesystem, &config));
        Mount();
    }

    void Mount()
    {
        LONGS_EQUAL(LFS_ERR_OK, lfs_mount(&filesystem, &config));
        file = SolidSyslogLittleFsFile_Create(&filesystem, fileCache, sizeof(fileCache));
        device = SolidSyslogFileBlockDevice_Create(file, STORE_PREFIX, STORE_BLOCK_BYTES);
        SolidSyslogBlockStoreConfig storeConfig = {};
        storeConfig.BlockDevice = device;
        storeConfig.MaxBlocks = STORE_MAX_BLOCKS;
        storeConfig.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
        store = SolidSyslogBlockStore_Create(&storeConfig);
    }

    /* Release every handle the library holds, as losing power would. The
     * storage is untouched. */
    void Unmount()
    {
        SolidSyslogBlockStore_Destroy(store);
        SolidSyslogFileBlockDevice_Destroy(device);
        SolidSyslogLittleFsFile_Destroy(file);
        store = nullptr;
        device = nullptr;
        file = nullptr;
        (void) lfs_unmount(&filesystem);
    }

    /* A reboot: the library state and the lfs_t go, the bytes stay. The
     * unmount and fresh mount are what actually discard LittleFS's in-RAM
     * state; zeroing the struct as well is belt-and-braces that no test
     * distinguishes, kept because it says what a reboot is. */
    void Reboot()
    {
        Unmount();
        std::memset(&filesystem, 0, sizeof(filesystem));
        LittleFsDisk_PowerOn(&disk);
        Mount();
    }

    [[nodiscard]] bool Write(const std::string& record) const
    {
        return SolidSyslogStore_Write(store, record.data(), record.size());
    }

    [[nodiscard]] std::vector<std::string> DrainUnsent() const
    {
        std::vector<std::string> records;
        char buffer[256] = {};
        size_t bytesRead = 0;
        while (SolidSyslogStore_ReadNextUnsent(store, buffer, sizeof(buffer), &bytesRead) && (bytesRead > 0))
        {
            records.emplace_back(buffer, bytesRead);
            SolidSyslogStore_MarkSent(store);
            bytesRead = 0;
        }
        return records;
    }
};

bool Contains(const std::vector<std::string>& records, const std::string& wanted)
{
    return std::any_of(
        records.begin(),
        records.end(),
        [&wanted](const std::string& record) { return record == wanted; }
    );
}

} // namespace

static Stack stack;

// clang-format off
TEST_GROUP(SolidSyslogLittleFsDurability)
{
    void setup() override
    {
        stack = Stack();
        stack.FormatAndMount();
    }

    void teardown() override
    {
        stack.Unmount();
    }
};

// clang-format on

TEST(SolidSyslogLittleFsDurability, RecordsSurviveAnOrderlyRemount)
{
    CHECK_TRUE(stack.Write("first"));
    CHECK_TRUE(stack.Write("second"));

    stack.Reboot();

    std::vector<std::string> recovered = stack.DrainUnsent();
    LONGS_EQUAL(2, recovered.size());
    CHECK_TRUE(Contains(recovered, "first"));
    CHECK_TRUE(Contains(recovered, "second"));
}

TEST(SolidSyslogLittleFsDurability, ARecordReportedWrittenSurvivesACleanCut)
{
    CHECK_TRUE(stack.Write("committed"));

    /* The cut lands inside the next record's write. The store must refuse to
       promise that record rather than report a write it could not commit. */
    LittleFsDisk_CutAfter(&stack.disk, 1, LITTLEFS_DISK_CUT_CLEAN);
    CHECK_FALSE(stack.Write("lost"));

    stack.Reboot();

    std::vector<std::string> recovered = stack.DrainUnsent();
    LONGS_EQUAL(1, recovered.size());
    CHECK_TRUE(Contains(recovered, "committed"));
    CHECK_FALSE(Contains(recovered, "lost"));
}

TEST(SolidSyslogLittleFsDurability, ARecordReportedWrittenSurvivesATornCut)
{
    CHECK_TRUE(stack.Write("committed"));

    /* A torn program leaves half a buffer on the media, which is the case
       LittleFS's copy-on-write commits exist to survive. */
    LittleFsDisk_CutAfter(&stack.disk, 1, LITTLEFS_DISK_CUT_TORN);
    CHECK_FALSE(stack.Write("torn"));

    stack.Reboot();

    std::vector<std::string> recovered = stack.DrainUnsent();
    LONGS_EQUAL(1, recovered.size());
    CHECK_TRUE(Contains(recovered, "committed"));
    CHECK_FALSE(Contains(recovered, "torn"));
}

TEST(SolidSyslogLittleFsDurability, AnInterruptedRecordNeverPresentsAsAPartialOne)
{
    CHECK_TRUE(stack.Write("committed"));
    LittleFsDisk_CutAfter(&stack.disk, 1, LITTLEFS_DISK_CUT_TORN);
    (void) stack.Write("interrupted-record-payload");

    stack.Reboot();

    /* Whatever survived must be a record the store wrote whole. A fragment of
       the interrupted payload presenting as a complete record is the failure
       this guards. */
    for (const std::string& record : stack.DrainUnsent())
    {
        CHECK_TRUE((record == "committed") || (record == "interrupted-record-payload"));
    }
}

TEST(SolidSyslogLittleFsDurability, TheCutActuallyFiredDuringTheWrite)
{
    CHECK_TRUE(stack.Write("committed"));

    LittleFsDisk_CutAfter(&stack.disk, 1, LITTLEFS_DISK_CUT_CLEAN);
    (void) stack.Write("lost");

    /* Without this the three tests above would pass against a device that never
       cut the power, and prove nothing. */
    CHECK_TRUE(LittleFsDisk_PowerLost(&stack.disk));
}
