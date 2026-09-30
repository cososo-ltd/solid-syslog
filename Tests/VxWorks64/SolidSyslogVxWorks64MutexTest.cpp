#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "vxWorks.h"

#include "semLib.h"

#include "SolidSyslogMutex.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogVxWorks64Mutex.h"
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

TEST(SolidSyslogVxWorks64Mutex, LockTakesTheCreatedSemaphore)
{
    SolidSyslogMutex_Lock(mutex);
    CALLED_FAKE(VxWorks64SemFake_SemTake, ONCE);
    POINTERS_EQUAL(VxWorks64SemFake_LastCreatedId(), VxWorks64SemFake_LastTakenId());
}

TEST(SolidSyslogVxWorks64Mutex, LockWaitsForever)
{
    SolidSyslogMutex_Lock(mutex);
    LONGS_EQUAL(WAIT_FOREVER, VxWorks64SemFake_LastTakeTimeout());
}

TEST(SolidSyslogVxWorks64Mutex, UnlockGivesTheCreatedSemaphore)
{
    SolidSyslogMutex_Unlock(mutex);
    CALLED_FAKE(VxWorks64SemFake_SemGive, ONCE);
    POINTERS_EQUAL(VxWorks64SemFake_LastCreatedId(), VxWorks64SemFake_LastGivenId());
}

TEST(SolidSyslogVxWorks64Mutex, DestroyDeletesTheCreatedSemaphore)
{
    SolidSyslogVxWorks64Mutex_Destroy(mutex);
    mutex = nullptr;

    CALLED_FAKE(VxWorks64SemFake_SemDelete, ONCE);
    POINTERS_EQUAL(VxWorks64SemFake_LastCreatedId(), VxWorks64SemFake_LastDeletedId());
}
