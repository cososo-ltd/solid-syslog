#include <stddef.h>
#include <string>

#include "CppUTest/TestHarness.h"
#include "ErrorHandlerFake.h"
#include "MutexFake.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogScratch.h"
#include "SolidSyslogSharedScratch.h"
#include "SolidSyslogSharedScratchErrors.h"
#include "SolidSyslogStackScratch.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"

using namespace CososoTesting;

static int userCalls;
static void* userContext;
static size_t userMessageBytes;
static SolidSyslogFormatterStorage* userStorage;
static std::string mutexSequenceSeenByUser;

static void SpyUser(SolidSyslogFormatterStorage* storage, size_t messageBytes, void* context)
{
    userCalls++;
    userContext = context;
    userMessageBytes = messageBytes;
    userStorage = storage;
    mutexSequenceSeenByUser = MutexFake_Sequence();
}

// clang-format off
TEST_GROUP(SolidSyslogSharedScratch)
{
    struct SolidSyslogScratch* scratch = nullptr;

    void setup() override
    {
        userCalls = 0;
        userContext = nullptr;
        userMessageBytes = 0;
        userStorage = nullptr;
        mutexSequenceSeenByUser.clear();
        scratch = SolidSyslogSharedScratch_Create(SolidSyslogNullMutex_Get());
    }

    void teardown() override
    {
        SolidSyslogSharedScratch_Destroy(scratch);
        MutexFake_Destroy();
    }

    void Use()
    {
        scratch->WithScratch(scratch, SpyUser, nullptr);
    }

    void RecreateOver(struct SolidSyslogMutex* mutex)
    {
        SolidSyslogSharedScratch_Destroy(scratch);
        scratch = SolidSyslogSharedScratch_Create(mutex);
    }
};

// clang-format on

TEST(SolidSyslogSharedScratch, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogSharedScratch, CreateReturnsAPooledInstanceNotTheStackScratch)
{
    CHECK(scratch != SolidSyslogStackScratch_Get());
}

TEST(SolidSyslogSharedScratch, WithScratchRunsUserOnce)
{
    Use();
    LONGS_EQUAL(1, userCalls);
}

TEST(SolidSyslogSharedScratch, WithScratchPassesContextThrough)
{
    int token = 0;
    scratch->WithScratch(scratch, SpyUser, &token);
    POINTERS_EQUAL(&token, userContext);
}

TEST(SolidSyslogSharedScratch, UserIsToldTheStorageHoldsAMaxSizeMessage)
{
    Use();
    LONGS_EQUAL(SOLIDSYSLOG_MAX_MESSAGE_SIZE, userMessageBytes);
}

TEST(SolidSyslogSharedScratch, StorageIsTheSameBufferOnEveryCall)
{
    Use();
    SolidSyslogFormatterStorage* first = userStorage;
    Use();
    POINTERS_EQUAL(first, userStorage);
}

TEST(SolidSyslogSharedScratch, WithScratchLocksThenUnlocksOnce)
{
    RecreateOver(MutexFake_Create());
    Use();
    STRCMP_EQUAL("LU", MutexFake_Sequence());
}

TEST(SolidSyslogSharedScratch, UserRunsWhileTheMutexIsHeld)
{
    RecreateOver(MutexFake_Create());
    Use();
    STRCMP_EQUAL("L", mutexSequenceSeenByUser.c_str());
}

TEST(SolidSyslogSharedScratch, NullMutexFallsBackToTheStackScratch)
{
    RecreateOver(nullptr);
    POINTERS_EQUAL(SolidSyslogStackScratch_Get(), scratch);
}

TEST(SolidSyslogSharedScratch, NullMutexReportsBadConfig)
{
    SolidSyslogSharedScratch_Destroy(scratch);
    ErrorHandlerFake_Install(nullptr);

    scratch = SolidSyslogSharedScratch_Create(nullptr);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogSharedScratchErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_SHARED_SCRATCH_ERROR_NULL_MUTEX
    );
}

TEST(SolidSyslogSharedScratch, DestroyWithNullHandleEmitsUnknownDestroyWarning)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogSharedScratch_Destroy(nullptr);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogSharedScratchErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_SHARED_SCRATCH_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogSharedScratch, DestroyingTheStackFallbackEmitsUnknownDestroyWarning)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogSharedScratch_Destroy(SolidSyslogStackScratch_Get());

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogSharedScratchErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_SHARED_SCRATCH_ERROR_UNKNOWN_DESTROY
    );
}

/* After Destroy the slot carries the stack scratch's vtable, so a stale
   handle still formats, on the stack, rather than through a NULL pointer. */
TEST(SolidSyslogSharedScratch, UseAfterDestroyFormatsOnTheStack)
{
    SolidSyslogSharedScratch_Destroy(scratch);

    Use();

    LONGS_EQUAL(1, userCalls);
    scratch = SolidSyslogSharedScratch_Create(SolidSyslogNullMutex_Get()); // for teardown
}

TEST(SolidSyslogSharedScratch, TheErrorSourceNamesItsDetailsThroughSharedScratchErrors)
{
    CHECK(SolidSyslogSharedScratchErrors_Name == SolidSyslogSharedScratchErrorSource.DetailName);
}

// Pool tests - the pool is sized by SOLIDSYSLOG_POOL_SIZE, one shared scratch
// per logger the build can hold, and overflow falls back to the stack scratch.
// Generic pool mechanics are covered by SolidSyslogPoolAllocatorTest.cpp.

// clang-format off
TEST_GROUP(SolidSyslogSharedScratchPool)
{
    struct SolidSyslogScratch* pooled[SOLIDSYSLOG_POOL_SIZE] = {};
    struct SolidSyslogScratch* overflow                      = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogSharedScratch_Destroy(handle);
            }
        }
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogSharedScratch_Create(SolidSyslogNullMutex_Get());
        }
    }
};

// clang-format on

TEST(SolidSyslogSharedScratchPool, OverflowReportsPoolExhausted)
{
    FillPool();
    ErrorHandlerFake_Install(nullptr);

    overflow = SolidSyslogSharedScratch_Create(SolidSyslogNullMutex_Get());

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogSharedScratchErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_SHARED_SCRATCH_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogSharedScratchPool, OverflowFallsBackToTheStackScratch)
{
    FillPool();

    overflow = SolidSyslogSharedScratch_Create(SolidSyslogNullMutex_Get());

    POINTERS_EQUAL(SolidSyslogStackScratch_Get(), overflow);
}
