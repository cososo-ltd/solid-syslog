#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

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
