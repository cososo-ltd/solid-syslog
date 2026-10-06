#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "BddTargetVxWorks64Store.h"
#include "VxWorks64FsFake.h"

// clang-format off
TEST_GROUP(BddTargetVxWorks64Store)
{
    void setup() override
    {
        VxWorks64FsFake_Reset();
    }
};

// clang-format on

TEST(BddTargetVxWorks64Store, MountLooksAtTheVolumesRoot)
{
    BddTargetVxWorks64Store_Mount();

    CALLED_FAKE(VxWorks64FsFake_Stat, ONCE);
    STRCMP_EQUAL("/ata0a/", VxWorks64FsFake_LastStatName());
}

TEST(BddTargetVxWorks64Store, MountOfAFormattedVolumeSucceeds)
{
    VxWorks64FsFake_FormatTheVolume();

    CHECK_TRUE(BddTargetVxWorks64Store_Mount());
}

TEST(BddTargetVxWorks64Store, MountLeavesAFormattedVolumeAlone)
{
    VxWorks64FsFake_FormatTheVolume();

    BddTargetVxWorks64Store_Mount();

    CALLED_FAKE(VxWorks64FsFake_DosFsVolFormat, NEVER);
}

TEST(BddTargetVxWorks64Store, MountFormatsABlankVolume)
{
    BddTargetVxWorks64Store_Mount();

    CALLED_FAKE(VxWorks64FsFake_DosFsVolFormat, ONCE);
    STRCMP_EQUAL("/ata0a", VxWorks64FsFake_LastFormatPath());
}

TEST(BddTargetVxWorks64Store, MountFormatsWithTheDefaultOptionsAndNoPrompt)
{
    BddTargetVxWorks64Store_Mount();

    LONGS_EQUAL(0, VxWorks64FsFake_LastFormatOptions());
    CHECK_TRUE(VxWorks64FsFake_LastFormatHadNoPrompt());
}

TEST(BddTargetVxWorks64Store, MountOfABlankVolumeSucceedsOnceFormatted)
{
    CHECK_TRUE(BddTargetVxWorks64Store_Mount());
}

TEST(BddTargetVxWorks64Store, MountFailsWhenTheFormatFails)
{
    VxWorks64FsFake_FailFormats();

    CHECK_FALSE(BddTargetVxWorks64Store_Mount());
}

TEST(BddTargetVxWorks64Store, MountFormatsAVolumeWhoseRootIsNotADirectory)
{
    VxWorks64FsFake_MakeTheRootAFile();

    BddTargetVxWorks64Store_Mount();

    CALLED_FAKE(VxWorks64FsFake_DosFsVolFormat, ONCE);
}
