#include "BddTargetBlockFiles.h"
#include "CppUTest/TestHarness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>

namespace
{
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
};
// clang-format on

TEST(BddTargetBlockFiles, NoBlocksPrintsTheBareHeader)
{
    STRCMP_EQUAL("BLOCKS:\n", PrintBlockFiles(nullptr, "/STORE", 0U).c_str());
}
