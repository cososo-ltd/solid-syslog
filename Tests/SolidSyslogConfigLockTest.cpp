#include "CppUTest/TestHarness.h"

#include "ConfigLockFake.h"
#include "SolidSyslogConfigLock.h"
#include "TestUtils.h"

using namespace CososoTesting;

static int testLockCallCount;
static int testUnlockCallCount;
static void* testLockContext;
static void* testUnlockContext;

static void TestLockRecordingContext(void* context)
{
    testLockContext = context;
}

static void TestUnlockRecordingContext(void* context)
{
    testUnlockContext = context;
}

static void TestLock(void* context)
{
    (void) context;
    testLockCallCount++;
}

static void TestUnlock(void* context)
{
    (void) context;
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

TEST(SolidSyslogConfigLock, LockFunctionReceivesInstalledContext)
{
    int context = 0;

    SolidSyslog_SetConfigLock(TestLockRecordingContext, nullptr, &context);
    SolidSyslog_LockConfig();

    POINTERS_EQUAL(&context, testLockContext);
}

TEST(SolidSyslogConfigLock, UnlockFunctionReceivesInstalledContext)
{
    int context = 0;

    SolidSyslog_SetConfigLock(nullptr, TestUnlockRecordingContext, &context);
    SolidSyslog_UnlockConfig();

    POINTERS_EQUAL(&context, testUnlockContext);
}

TEST(SolidSyslogConfigLock, SetConfigLockWithNullLockRestoresDefault)
{
    SolidSyslog_SetConfigLock(TestLock, TestUnlock, nullptr);

    SolidSyslog_SetConfigLock(nullptr, TestUnlock, nullptr);
    SolidSyslog_LockConfig();

    CALLED_FUNCTION(testLock, NEVER);
}

TEST(SolidSyslogConfigLock, SetConfigLockWithNullUnlockRestoresDefault)
{
    SolidSyslog_SetConfigLock(TestLock, TestUnlock, nullptr);

    SolidSyslog_SetConfigLock(TestLock, nullptr, nullptr);
    SolidSyslog_UnlockConfig();

    CALLED_FUNCTION(testUnlock, NEVER);
}
