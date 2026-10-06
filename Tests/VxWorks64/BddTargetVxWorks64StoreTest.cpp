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
    BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_DOSFS);

    CALLED_FAKE(VxWorks64FsFake_Stat, ONCE);
    STRCMP_EQUAL("/ata0a/", VxWorks64FsFake_LastStatName());
}

TEST(BddTargetVxWorks64Store, MountOfADosFsVolumeSucceedsWhenDosFsIsChosen)
{
    VxWorks64FsFake_FormatTheVolumeAsDosFs();

    CHECK_TRUE(BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_DOSFS));
}

TEST(BddTargetVxWorks64Store, MountLeavesADosFsVolumeAloneWhenDosFsIsChosen)
{
    VxWorks64FsFake_FormatTheVolumeAsDosFs();

    BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_DOSFS);

    CALLED_FAKE(VxWorks64FsFake_DosFsVolFormat, NEVER);
}

TEST(BddTargetVxWorks64Store, MountFormatsABlankVolumeWithDosFsWhenDosFsIsChosen)
{
    BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_DOSFS);

    CALLED_FAKE(VxWorks64FsFake_DosFsVolFormat, ONCE);
    STRCMP_EQUAL("/ata0a", VxWorks64FsFake_LastFormatPath());
}

TEST(BddTargetVxWorks64Store, MountFormatsDosFsWithTheDefaultOptionsAndNoPrompt)
{
    BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_DOSFS);

    LONGS_EQUAL(0, VxWorks64FsFake_LastFormatOptions());
    CHECK_TRUE(VxWorks64FsFake_LastFormatHadNoPrompt());
}

TEST(BddTargetVxWorks64Store, MountOfABlankVolumeSucceedsOnceFormatted)
{
    CHECK_TRUE(BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_DOSFS));
}

TEST(BddTargetVxWorks64Store, MountFailsWhenTheDosFsFormatFails)
{
    VxWorks64FsFake_FailFormats();

    CHECK_FALSE(BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_DOSFS));
}

TEST(BddTargetVxWorks64Store, MountFormatsAVolumeWhoseRootIsNotADirectory)
{
    VxWorks64FsFake_MakeTheRootAFile();

    BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_DOSFS);

    CALLED_FAKE(VxWorks64FsFake_DosFsVolFormat, ONCE);
}

TEST(BddTargetVxWorks64Store, MountFormatsABlankVolumeWithHrfsWhenHrfsIsChosen)
{
    BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_HRFS);

    CALLED_FAKE(VxWorks64FsFake_HrfsFormat, ONCE);
    STRCMP_EQUAL("/ata0a", VxWorks64FsFake_LastFormatPath());
    CALLED_FAKE(VxWorks64FsFake_DosFsVolFormat, NEVER);
}

TEST(BddTargetVxWorks64Store, MountLeavesTheHrfsLayoutToTheFormatter)
{
    BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_HRFS);

    CHECK_TRUE(VxWorks64FsFake_LastHrfsFormatTookTheDefaults());
}

TEST(BddTargetVxWorks64Store, MountFailsWhenTheHrfsFormatFails)
{
    VxWorks64FsFake_FailFormats();

    CHECK_FALSE(BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_HRFS));
}

TEST(BddTargetVxWorks64Store, MountLeavesAnHrfsVolumeAloneWhenHrfsIsChosen)
{
    VxWorks64FsFake_FormatTheVolumeAsHrfs();

    CHECK_TRUE(BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_HRFS));
    CALLED_FAKE(VxWorks64FsFake_HrfsFormat, NEVER);
}

TEST(BddTargetVxWorks64Store, MountRefusesADosFsVolumeWhenHrfsIsChosen)
{
    VxWorks64FsFake_FormatTheVolumeAsDosFs();

    CHECK_FALSE(BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_HRFS));
}

TEST(BddTargetVxWorks64Store, MountRefusesAnHrfsVolumeWhenDosFsIsChosen)
{
    VxWorks64FsFake_FormatTheVolumeAsHrfs();

    CHECK_FALSE(BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_DOSFS));
}

TEST(BddTargetVxWorks64Store, MountDoesNotReformatAVolumeOfTheOtherKind)
{
    VxWorks64FsFake_FormatTheVolumeAsDosFs();

    BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_HRFS);

    CALLED_FAKE(VxWorks64FsFake_HrfsFormat, NEVER);
    CALLED_FAKE(VxWorks64FsFake_DosFsVolFormat, NEVER);
}

TEST(BddTargetVxWorks64Store, MountAsksDosFsWhetherItOwnsTheVolume)
{
    VxWorks64FsFake_FormatTheVolumeAsHrfs();

    BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_HRFS);

    CALLED_FAKE(VxWorks64FsFake_DosFsVolDescGet, ONCE);
    STRCMP_EQUAL("/ata0a", VxWorks64FsFake_LastVolDescGetName());
    CHECK_TRUE(VxWorks64FsFake_LastVolDescGetHadNoTail());
}
