#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <string>

#include "BddTargetBlockFiles.h"
#include "SolidSyslogBlockDeviceDefinition.h"
#include "CppUTest/TestHarness.h"

namespace
{
/* A block device that answers how many blocks it has and which are present;
   nothing else is ever called on it. */
std::set<size_t> presentBlocks;
size_t blockCount = 0U;

bool ExistsInSet(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    (void) base;
    return presentBlocks.count(blockIndex) > 0;
}

size_t CountOfBlocks(struct SolidSyslogBlockDevice* base)
{
    (void) base;
    return blockCount;
}

struct SolidSyslogBlockDevice listDevice = {};

std::string PrintBlocks()
{
    char* buffer = nullptr;
    size_t length = 0U;
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- open_memstream/fclose is POSIX C; buffer freed below
    FILE* out = open_memstream(&buffer, &length);
    BddTargetBlockFiles_Print(out, &listDevice);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- see above
    fclose(out);
    std::string captured(buffer, length);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc) -- open_memstream allocates with malloc
    free(buffer);
    return captured;
}
} // namespace

#define CHECK_BLOCKS_LISTED(expected) STRCMP_EQUAL(expected, PrintBlocks().c_str())

// clang-format off
TEST_GROUP(BddTargetBlockFiles)
{
    void setup() override
    {
        presentBlocks.clear();
        blockCount = 100U;
        listDevice.Exists = ExistsInSet;
        listDevice.GetBlockCount = CountOfBlocks;
    }

    void teardown() override
    {
        presentBlocks.clear();
    }
};
// clang-format on

TEST(BddTargetBlockFiles, NoBlocksPrintsTheBareHeader)
{
    CHECK_BLOCKS_LISTED("BLOCKS:\n");
}

TEST(BddTargetBlockFiles, ADeviceOfNoBlocksPrintsTheBareHeader)
{
    blockCount = 0U;
    presentBlocks = {0};

    CHECK_BLOCKS_LISTED("BLOCKS:\n");
}

TEST(BddTargetBlockFiles, OnePresentBlockIsListedByItsIndex)
{
    presentBlocks = {0};

    CHECK_BLOCKS_LISTED("BLOCKS: 00\n");
}

TEST(BddTargetBlockFiles, PresentBlocksAreListedInIndexOrderWithGapsLeftOut)
{
    presentBlocks = {2, 0};

    CHECK_BLOCKS_LISTED("BLOCKS: 00 02\n");
}

TEST(BddTargetBlockFiles, TheDevicesLastBlockIsListed)
{
    presentBlocks = {99};

    CHECK_BLOCKS_LISTED("BLOCKS: 99\n");
}

TEST(BddTargetBlockFiles, ABlockPastTheDevicesCountIsNotAskedFor)
{
    blockCount = 8U;
    presentBlocks = {7, 8};

    CHECK_BLOCKS_LISTED("BLOCKS: 07\n");
}

TEST(BddTargetBlockFiles, ADeviceOfMoreThanOneHundredBlocksListsThemAll)
{
    blockCount = 256U;
    presentBlocks = {99, 100, 255};

    CHECK_BLOCKS_LISTED("BLOCKS: 99 100 255\n");
}

/* Test list (ZOMBIES order):
 *  Z  [x] NoBlocksPrintsTheBareHeader
 *  Z  [x] ADeviceOfNoBlocksPrintsTheBareHeader
 *  O  [x] OnePresentBlockIsListedByItsIndex
 *  M  [x] PresentBlocksAreListedInIndexOrderWithGapsLeftOut
 *  B  [x] TheDevicesLastBlockIsListed
 *  B  [x] ABlockPastTheDevicesCountIsNotAskedFor
 *  B  [x] ADeviceOfMoreThanOneHundredBlocksListsThemAll
 */
