#include <stddef.h>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogScratch.h"
#include "SolidSyslogStackScratch.h"
#include "SolidSyslogTunables.h"

static int userCalls;
static void* userContext;
static size_t userMessageBytes;
static size_t userFormattedLength;

static void SpyUser(SolidSyslogFormatterStorage* /* storage */, size_t messageBytes, void* context)
{
    userCalls++;
    userContext = context;
    userMessageBytes = messageBytes;
}

static void FillingUser(SolidSyslogFormatterStorage* storage, size_t messageBytes, void* /* context */)
{
    struct SolidSyslogFormatter* formatter = SolidSyslogFormatter_Create(storage, messageBytes);
    for (size_t i = 0; i < SOLIDSYSLOG_MAX_MESSAGE_SIZE; i++)
    {
        SolidSyslogFormatter_AsciiCharacter(formatter, 'a');
    }
    userFormattedLength = SolidSyslogFormatter_Length(formatter);
}

// clang-format off
TEST_GROUP(SolidSyslogStackScratch)
{
    struct SolidSyslogScratch* scratch = nullptr;

    void setup() override
    {
        userCalls = 0;
        userContext = nullptr;
        userMessageBytes = 0;
        userFormattedLength = 0;
        scratch = SolidSyslogStackScratch_Get();
    }
};

// clang-format on

TEST(SolidSyslogStackScratch, GetReturnsSameInstance)
{
    POINTERS_EQUAL(scratch, SolidSyslogStackScratch_Get());
}

TEST(SolidSyslogStackScratch, WithScratchRunsUserOnce)
{
    scratch->WithScratch(scratch, SpyUser, nullptr);
    LONGS_EQUAL(1, userCalls);
}

TEST(SolidSyslogStackScratch, WithScratchPassesContextThrough)
{
    int token = 0;
    scratch->WithScratch(scratch, SpyUser, &token);
    POINTERS_EQUAL(&token, userContext);
}

TEST(SolidSyslogStackScratch, UserIsToldTheStorageHoldsAMaxSizeMessage)
{
    scratch->WithScratch(scratch, SpyUser, nullptr);
    LONGS_EQUAL(SOLIDSYSLOG_MAX_MESSAGE_SIZE, userMessageBytes);
}

/* The formatter keeps one byte of whatever it is given for its terminator, so
   the most it ever writes at this size is one short of it. */
TEST(SolidSyslogStackScratch, StorageTakesEverythingAMaxSizeFormatterWrites)
{
    scratch->WithScratch(scratch, FillingUser, nullptr);
    LONGS_EQUAL(SOLIDSYSLOG_MAX_MESSAGE_SIZE - 1U, userFormattedLength);
}
