#include "BddTargetBlockFiles.h"
#include "SolidSyslogFileDefinition.h"
#include "CppUTest/TestHarness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

namespace
{
/* A File whose Exists answers from a list of present paths; nothing else is
   ever called on it. */
const char* presentPaths[4] = {};

bool ExistsInList(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    for (const char* present : presentPaths)
    {
        if ((present != nullptr) && (strcmp(present, path) == 0))
        {
            return true;
        }
    }
    return false;
}

struct SolidSyslogFile listFile = {};

std::string PrintBlockFiles(struct SolidSyslogFile* file, const char* prefix, size_t maxBlocks)
{
    char* buffer = nullptr;
    size_t length = 0U;
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- open_memstream/fclose is POSIX C; buffer freed below
    FILE* out = open_memstream(&buffer, &length);
    BddTargetBlockFiles_Print(out, file, prefix, maxBlocks);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- see above
    fclose(out);
    std::string captured(buffer, length);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc) -- open_memstream allocates with malloc
    free(buffer);
    return captured;
}
} // namespace

// clang-format off
TEST_GROUP(BddTargetBlockFiles)
{
    void setup() override
    {
        memset(presentPaths, 0, sizeof(presentPaths));
        listFile.Exists = ExistsInList;
    }
};
// clang-format on

TEST(BddTargetBlockFiles, NoBlocksPrintsTheBareHeader)
{
    STRCMP_EQUAL("BLOCKS:\n", PrintBlockFiles(nullptr, "/STORE", 0U).c_str());
}

TEST(BddTargetBlockFiles, OnePresentBlockIsListedByItsIndex)
{
    presentPaths[0] = "/STORE00.log";

    STRCMP_EQUAL("BLOCKS: 00\n", PrintBlockFiles(&listFile, "/STORE", 1U).c_str());
}
