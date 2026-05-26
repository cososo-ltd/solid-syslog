#include "CppUTest/TestHarness.h"

#include "ConfigLockFake.h"
#include "SolidSyslogConfigLock.h"
#include "TestUtils.h"

using namespace CososoTesting;

static int testLockCallCount;
static int testUnlockCallCount;

static void TestLock()
{
    testLockCallCount++;
}

static void TestUnlock()
{
    testUnlockCallCount++;
}

// clang-format off
TEST_GROUP(SolidSyslogConfigLock)
{
    void setup() override
    {
        testLockCallCount = 0;
        testUnlockCallCount = 0;
    }

    void teardown() override
    {
        ConfigLockFake_Uninstall();
    }
};

// clang-format on

TEST(SolidSyslogConfigLock, LockConfigWithDefaultHandlerDoesNotCrash)
{
    SolidSyslog_LockConfig();
}

TEST(SolidSyslogConfigLock, UnlockConfigWithDefaultHandlerDoesNotCrash)
{
    SolidSyslog_UnlockConfig();
}

TEST(SolidSyslogConfigLock, InstalledLockFunctionIsCalledByLockConfig)
{
    ConfigLockFake_Install();

    SolidSyslog_LockConfig();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
}

TEST(SolidSyslogConfigLock, InstalledUnlockFunctionIsCalledByUnlockConfig)
{
    ConfigLockFake_Install();

    SolidSyslog_UnlockConfig();

    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogConfigLock, SetConfigLockWithNullLockRestoresDefault)
{
    SolidSyslog_SetConfigLock(TestLock, TestUnlock);

    SolidSyslog_SetConfigLock(nullptr, TestUnlock);
    SolidSyslog_LockConfig();

    CALLED_FUNCTION(testLock, NEVER);
}

TEST(SolidSyslogConfigLock, SetConfigLockWithNullUnlockRestoresDefault)
{
    SolidSyslog_SetConfigLock(TestLock, TestUnlock);

    SolidSyslog_SetConfigLock(TestLock, nullptr);
    SolidSyslog_UnlockConfig();

    CALLED_FUNCTION(testUnlock, NEVER);
}
