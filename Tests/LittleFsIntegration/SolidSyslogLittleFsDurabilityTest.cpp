#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "CppUTest/TestHarness.h"

#include "LittleFsStoreStack.h"

/* The claim under test is the File contract's: a true return from Write means
 * the bytes are on the media, and the BlockStore treats that as surviving a
 * power loss. The unit suite proves the adapter calls lfs_file_sync against a
 * fake; only real LittleFS over a device that can lose power proves the bytes
 * are still there afterwards. */

namespace
{

bool Contains(const std::vector<std::string>& records, const std::string& wanted)
{
    return std::any_of(
        records.begin(),
        records.end(),
        [&wanted](const std::string& record) { return record == wanted; }
    );
}

} // namespace

static LittleFsStoreStack stack;

// clang-format off
TEST_GROUP(SolidSyslogLittleFsDurability)
{
    void setup() override
    {
        stack = LittleFsStoreStack();
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
