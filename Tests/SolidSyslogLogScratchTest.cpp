#include <stddef.h>
#include <string>

#include "CppUTest/TestHarness.h"
#include "ScratchFake.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogPassthroughBuffer.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSharedScratch.h"

// clang-format off
TEST_GROUP(SolidSyslogLogScratch)
{
    struct SolidSyslogSender* fakeSender = nullptr;
    struct SolidSyslogBuffer* buffer     = nullptr;
    struct SolidSyslog*       solidSyslog = nullptr;
    SolidSyslogMessage        message     = {SOLIDSYSLOG_FACILITY_LOCAL0, SOLIDSYSLOG_SEVERITY_INFORMATIONAL, nullptr, "hello"};

    void setup() override
    {
        fakeSender = SenderFake_Create();
        buffer     = SolidSyslogPassthroughBuffer_Create(fakeSender);
    }

    void teardown() override
    {
        SolidSyslog_Destroy(solidSyslog);
        SolidSyslogPassthroughBuffer_Destroy(buffer);
        SenderFake_Destroy(fakeSender);
        ScratchFake_Destroy();
    }

    void CreateWith(struct SolidSyslogScratch* scratch)
    {
        SolidSyslogConfig config = {};
        config.Buffer            = buffer;
        config.Sender            = fakeSender;
        config.Scratch           = scratch;
        solidSyslog = SolidSyslog_Create(&config);
    }

    std::string LogAndTakeRecord()
    {
        SolidSyslog_Log(solidSyslog, &message);
        return SenderFake_LastBufferAsString(fakeSender);
    }
};

// clang-format on

TEST(SolidSyslogLogScratch, LogFormatsThroughTheInjectedScratch)
{
    CreateWith(ScratchFake_Create());
    SolidSyslog_Log(solidSyslog, &message);
    LONGS_EQUAL(1, ScratchFake_CallCount());
}

TEST(SolidSyslogLogScratch, NullScratchStillLogs)
{
    CreateWith(nullptr);
    SolidSyslog_Log(solidSyslog, &message);
    CHECK(SenderFake_LastSize(fakeSender) > 0U);
}

TEST(SolidSyslogLogScratch, NullMessageNeverReachesTheScratch)
{
    CreateWith(ScratchFake_Create());
    SolidSyslog_Log(solidSyslog, nullptr);
    LONGS_EQUAL(0, ScratchFake_CallCount());
}

/* The formatter bounds its writes to the size the scratch declares, keeping
   one byte for its terminator, so a short scratch truncates and never overruns. */
TEST(SolidSyslogLogScratch, UndersizedScratchTruncatesTheRecord)
{
    SolidSyslogMessage longMessage = message;
    longMessage.Msg = "a message that runs well past the thirty-two bytes the scratch offers";
    CreateWith(ScratchFake_Create());
    ScratchFake_SetMessageBytes(32U);
    SolidSyslog_Log(solidSyslog, &longMessage);
    LONGS_EQUAL(31U, SenderFake_LastSize(fakeSender));
}

TEST(SolidSyslogLogScratch, InjectedScratchProducesTheSameRecordAsTheDefault)
{
    CreateWith(nullptr);
    std::string viaDefault = LogAndTakeRecord();
    SolidSyslog_Destroy(solidSyslog);

    CreateWith(ScratchFake_Create());
    STRCMP_EQUAL(viaDefault.c_str(), LogAndTakeRecord().c_str());
}

TEST(SolidSyslogLogScratch, SharedScratchProducesTheSameRecordAsTheDefault)
{
    CreateWith(nullptr);
    std::string viaDefault = LogAndTakeRecord();
    SolidSyslog_Destroy(solidSyslog);

    struct SolidSyslogScratch* shared = SolidSyslogSharedScratch_Create(SolidSyslogNullMutex_Get());
    CreateWith(shared);
    STRCMP_EQUAL(viaDefault.c_str(), LogAndTakeRecord().c_str());
    SolidSyslogSharedScratch_Destroy(shared);
}
