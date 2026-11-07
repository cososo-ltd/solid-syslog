#include <stddef.h>
#include <string>

#include "CppUTest/TestHarness.h"

#include "AddressFake.h"
#include "ResolverFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPassthroughBuffer.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogServiceStatus.h"
#include "SolidSyslogStreamSender.h"
#include "StreamFake.h"
#include "SyslogFieldParser.h"

struct SolidSyslog;
struct SolidSyslogBuffer;
struct SolidSyslogSender;
struct SolidSyslogStream;

/* The hardening path's passthrough wiring - a passthrough buffer over a
 * stream sender and no store - driven as a single-task integration drives
 * it: Service on the logging task, after each Log. The stream fake keeps
 * every frame it accepted, so what the collector saw, and in what order, can
 * be read back. */
// clang-format off
TEST_GROUP(SolidSyslogPassthroughReport)
{
    struct SolidSyslogStream* stream = nullptr;
    struct SolidSyslogSender* sender = nullptr;
    struct SolidSyslogBuffer* buffer = nullptr;
    struct SolidSyslog* solidSyslog = nullptr;
    struct SolidSyslogMessage message = {SOLIDSYSLOG_FACILITY_USER, SOLIDSYSLOG_SEVERITY_INFORMATIONAL, "LOGGED", "x"};

    void setup() override
    {
        ResolverFake_Reset();
        stream = StreamFake_Create();
        struct SolidSyslogStreamSenderConfig senderConfig = {};
        senderConfig.Resolver = ResolverFake_Get();
        senderConfig.Stream = stream;
        senderConfig.Address = AddressFake_Get();
        sender = SolidSyslogStreamSender_Create(&senderConfig);
        buffer = SolidSyslogPassthroughBuffer_Create(sender);
        struct SolidSyslogConfig config = {};
        config.Buffer = buffer;
        config.Sender = sender;
        config.Store = SolidSyslogNullStore_Get();
        solidSyslog = SolidSyslog_Create(&config);
    }

    void teardown() override
    {
        SolidSyslog_Destroy(solidSyslog);
        SolidSyslogPassthroughBuffer_Destroy(buffer);
        SolidSyslogStreamSender_Destroy(sender);
        StreamFake_Destroy(stream);
    }

    enum SolidSyslogServiceStatus LogThenService()
    {
        SolidSyslog_Log(solidSyslog, &message);
        return SolidSyslog_Service(solidSyslog);
    }

    [[nodiscard]] std::string FrameMsgId(size_t index) const
    {
        return SyslogField(StreamFake_Frame(stream, index), SYSLOG_FIELD_MSGID);
    }
};

// clang-format on

TEST(SolidSyslogPassthroughReport, ARecordTheCollectorTakesRaisesNoReport)
{
    LogThenService();

    LONGS_EQUAL(1, StreamFake_FrameCount(stream));
    STRCMP_EQUAL("LOGGED", FrameMsgId(0).c_str());
}

TEST(SolidSyslogPassthroughReport, ARecordLoggedWhileTheCollectorIsDownIsLostAndTheReportOfItIsHeld)
{
    StreamFake_SetSendFails(stream, true);

    enum SolidSyslogServiceStatus status = LogThenService();

    LONGS_EQUAL(0, StreamFake_FrameCount(stream));
    LONGS_EQUAL(SOLIDSYSLOG_SERVICE_BLOCKED, status);
}

TEST(SolidSyslogPassthroughReport, TheHeldReportFollowsTheFirstRecordTheCollectorTakesAgain)
{
    StreamFake_SetSendFails(stream, true);
    LogThenService();
    StreamFake_SetSendFails(stream, false);

    LogThenService();

    LONGS_EQUAL(3, StreamFake_FrameCount(stream));
    STRCMP_EQUAL("LOGGED", FrameMsgId(0).c_str());
    STRCMP_EQUAL("SENDER_DELIVERY_FAILED", FrameMsgId(1).c_str());
    STRCMP_EQUAL("SENDER_DELIVERY_RESTORED", FrameMsgId(2).c_str());
}

TEST(SolidSyslogPassthroughReport, AnOutageIsReportedOnceHoweverManyRecordsItLost)
{
    StreamFake_SetSendFails(stream, true);
    LogThenService();
    LogThenService();
    LogThenService();
    StreamFake_SetSendFails(stream, false);

    LogThenService();

    LONGS_EQUAL(3, StreamFake_FrameCount(stream));
}

TEST(SolidSyslogPassthroughReport, AServiceCallWithNoLogLandsTheHeldReportOnceTheCollectorIsBack)
{
    StreamFake_SetSendFails(stream, true);
    LogThenService();
    StreamFake_SetSendFails(stream, false);

    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(2, StreamFake_FrameCount(stream));
    STRCMP_EQUAL("SENDER_DELIVERY_FAILED", FrameMsgId(0).c_str());
    STRCMP_EQUAL("SENDER_DELIVERY_RESTORED", FrameMsgId(1).c_str());
}

TEST(SolidSyslogPassthroughReport, ARefusedHandshakeIsReportedAheadOfTheOutageOnceThePeerIsTrusted)
{
    StreamFake_RefuseHandshake(stream, true);
    LogThenService();
    StreamFake_RefuseHandshake(stream, false);

    LogThenService();

    LONGS_EQUAL(4, StreamFake_FrameCount(stream));
    STRCMP_EQUAL("LOGGED", FrameMsgId(0).c_str());
    STRCMP_EQUAL("TLS_STREAM_HANDSHAKE_FAILED", FrameMsgId(1).c_str());
    STRCMP_EQUAL("SENDER_DELIVERY_FAILED", FrameMsgId(2).c_str());
    STRCMP_EQUAL("SENDER_DELIVERY_RESTORED", FrameMsgId(3).c_str());
}
