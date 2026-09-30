#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "SolidSyslogNullMutex.h"
#include "SolidSyslogVxWorks64Mutex.h"
#include "semLib.h"
#include "VxWorks64SemFake.h"

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64Mutex)
{
    struct SolidSyslogMutex* mutex = nullptr;

    void setup() override
    {
        VxWorks64SemFake_Reset();
        mutex = SolidSyslogVxWorks64Mutex_Create();
    }

    void teardown() override
    {
        SolidSyslogVxWorks64Mutex_Destroy(mutex);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Mutex, CreateCallsSemMCreateOnce)
{
    CALLED_FAKE(VxWorks64SemFake_SemMCreate, ONCE);
}

TEST(SolidSyslogVxWorks64Mutex, CreateAsksForPriorityInheritanceAndDeletionSafety)
{
    LONGS_EQUAL(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE, VxWorks64SemFake_LastSemMCreateOptions());
}

TEST(SolidSyslogVxWorks64Mutex, CreateReturnsAMutexThatIsNotTheNullMutex)
{
    CHECK(mutex != SolidSyslogNullMutex_Get());
}
