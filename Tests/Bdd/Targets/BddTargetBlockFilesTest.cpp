#include "BddTargetBlockFiles.h"
#include "SolidSyslogFileDefinition.h"
#include "CppUTest/TestHarness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <iterator>
#include <string>

namespace
{
/* A File whose Exists answers from a list of present paths; nothing else is
   ever called on it. */
const char* presentPaths[4] = {};

bool ExistsInList(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    return std::any_of(
        std::begin(presentPaths),
        std::end(presentPaths),
        [path](const char* present) { return (present != nullptr) && (strcmp(present, path) == 0); }
    );
}

struct SolidSyslogFile listFile = {};

std::string PrintBlockFiles(struct SolidSyslogFile* file, const char* prefix)
{
    char* buffer = nullptr;
    size_t length = 0U;
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- open_memstream/fclose is POSIX C; buffer freed below
    FILE* out = open_memstream(&buffer, &length);
    BddTargetBlockFiles_Print(out, file, prefix);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- see above
    fclose(out);
    std::string captured(buffer, length);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc) -- open_memstream allocates with malloc
    free(buffer);
    return captured;
}
} // namespace

#define CHECK_BLOCKS_LISTED(expected, file, prefix) STRCMP_EQUAL(expected, PrintBlockFiles(file, prefix).c_str())

// clang-format off
TEST_GROUP(BddTargetBlockFiles)
{
    void setup() override
    {
        std::fill(std::begin(presentPaths), std::end(presentPaths), nullptr);
        listFile.Exists = ExistsInList;
    }
};
// clang-format on

TEST(BddTargetBlockFiles, NoBlocksPrintsTheBareHeader)
{
    CHECK_BLOCKS_LISTED("BLOCKS:\n", &listFile, "/STORE");
}

TEST(BddTargetBlockFiles, OnePresentBlockIsListedByItsIndex)
{
    presentPaths[0] = "/STORE00.log";

    CHECK_BLOCKS_LISTED("BLOCKS: 00\n", &listFile, "/STORE");
}

TEST(BddTargetBlockFiles, PresentBlocksAreListedInIndexOrderWithGapsLeftOut)
{
    presentPaths[0] = "/STORE02.log";
    presentPaths[1] = "/STORE00.log";

    CHECK_BLOCKS_LISTED("BLOCKS: 00 02\n", &listFile, "/STORE");
}

TEST(BddTargetBlockFiles, TheLastIndexTheSequenceCanUseIsListed)
{
    presentPaths[0] = "/STORE99.log";

    CHECK_BLOCKS_LISTED("BLOCKS: 99\n", &listFile, "/STORE");
}

TEST(BddTargetBlockFiles, AnIndexOfTenOrMoreKeepsTwoDigits)
{
    presentPaths[0] = "/ata0a/STORE10.log";

    CHECK_BLOCKS_LISTED("BLOCKS: 10\n", &listFile, "/ata0a/STORE");
}

/* Test list (ZOMBIES order):
 *  Z  [x] NoBlocksPrintsTheBareHeader
 *  O  [x] OnePresentBlockIsListedByItsIndex
 *  M  [x] PresentBlocksAreListedInIndexOrderWithGapsLeftOut
 *  B  [x] TheLastIndexTheSequenceCanUseIsListed
 *  B  [x] AnIndexOfTenOrMoreKeepsTwoDigits
 */
