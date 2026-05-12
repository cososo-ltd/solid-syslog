/* Slice 1 (chore) plumbs the FatFs host TDD build. The first real test
 * group (TEST_GROUP(SolidSyslogFatFsFile)) lands at slice 2 alongside
 * Platform/FatFs/Source/SolidSyslogFatFsFile.c — at that point this
 * plumbing group is removed.
 *
 * The single test below asserts the revision contract that ff.h enforces
 * with #error at compile time: it requires this TU to find our
 * FatFsFakes/Interface/ffconf.h first on the include path, reach
 * $FATFS_PATH/source/ff.h, and agree on the revision (80386 ↔ R0.16).
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ff.h"
}

TEST_GROUP(FatFsPlumbing){};

TEST(FatFsPlumbing, FfConfRevisionMatchesFatFsHeader)
{
    LONGS_EQUAL(FF_DEFINED, FFCONF_DEF);
}
