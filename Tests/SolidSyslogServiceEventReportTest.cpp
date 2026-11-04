#include <stdint.h>
#include <string>

#include "CppUTest/TestHarness.h"

#include "AddressFake.h"
#include "ResolverFake.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPassthroughBuffer.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSdElement.h"
#include "SolidSyslogServiceStatus.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogStructuredDataDefinition.h"
#include "StoreFake.h"
#include "StreamFake.h"
#include "StringFake.h"
#include "SyslogFieldParser.h"
#include "TestUtils.h"

using namespace CososoTesting;

struct SolidSyslog;
struct SolidSyslogBuffer;
struct SolidSyslogSdElement;
struct SolidSyslogSender;
struct SolidSyslogStore;
struct SolidSyslogStream;

static const struct SolidSyslogErrorSource TEST_SOURCE = {"TestSource", nullptr};
static const int32_t TEST_DETAIL = 7;

/* The plain buffer loss most tests report: the test source and detail, with
 * no native code. */
static void RaiseBufferLoss()
{
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &TEST_SOURCE, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, TEST_DETAIL);
}

static void SdSpyFormat(struct SolidSyslogStructuredData* /* self */, struct SolidSyslogSdElement* element)
{
    SolidSyslogSdElement_Begin(element, "spy", 0U);
    SolidSyslogSdElement_End(element);
}

static struct SolidSyslogStructuredData sdSpy = {SdSpyFormat};

/* A logger with its own identity and one SD of its own, over a passthrough
 * buffer and a store that keeps nothing: a Log goes straight to the sender
 * fake, so the only records Service sends are its reports. */
// clang-format off
TEST_GROUP(SolidSyslogServiceEventReport)
{
    struct SolidSyslogSender* fakeSender = nullptr;
    struct SolidSyslogBuffer* buffer = nullptr;
    struct SolidSyslog* solidSyslog = nullptr;
    struct SolidSyslogStructuredData* sds[1] = {&sdSpy};
    struct SolidSyslogConfig config = {};

    void setup() override
    {
        fakeSender = SenderFake_Create();
        buffer = SolidSyslogPassthroughBuffer_Create(fakeSender);
        StringFake_Reset();
        config.Buffer = buffer;
        config.Sender = fakeSender;
        config.Store = SolidSyslogNullStore_Get();
        config.GetHostname = StringFake_GetHostname;
        config.GetAppName = StringFake_GetAppName;
        config.GetProcessId = StringFake_GetProcessId;
        config.Sd = sds;
        config.SdCount = 1U;
        solidSyslog = SolidSyslog_Create(&config);
    }

    void teardown() override
    {
        SolidSyslog_Destroy(solidSyslog);
        SolidSyslogPassthroughBuffer_Destroy(buffer);
        SenderFake_Destroy(fakeSender);
    }

    [[nodiscard]] std::string LastField(int field) const
    {
        return SyslogField(SenderFake_LastBufferAsString(fakeSender), field);
    }

    [[nodiscard]] std::string LastMsg() const
    {
        return SyslogMsg(SenderFake_LastBufferAsString(fakeSender));
    }
};

// clang-format on

TEST(SolidSyslogServiceEventReport, APendingBufferLossIsSentByTheNextService)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}

TEST(SolidSyslogServiceEventReport, TheReportIsAtFacilitySyslogAndTheEventsSeverity)
{
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, &TEST_SOURCE, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, TEST_DETAIL);

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL("<44>1", LastField(SYSLOG_FIELD_HEADER).c_str());
}

TEST(SolidSyslogServiceEventReport, TheReportCarriesTheLoggersOwnHostnameAppNameAndProcid)
{
    StringFake_SetHostname("TestHost");
    StringFake_SetAppName("TestApp");
    StringFake_SetProcessId("42");
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL("TestHost", LastField(SYSLOG_FIELD_HOSTNAME).c_str());
    STRCMP_EQUAL("TestApp", LastField(SYSLOG_FIELD_APP_NAME).c_str());
    STRCMP_EQUAL("42", LastField(SYSLOG_FIELD_PROCID).c_str());
}

TEST(SolidSyslogServiceEventReport, TheReportsMsgIdIsTheCategoryName)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL("BUFFER_RECORDS_LOST", LastField(SYSLOG_FIELD_MSGID).c_str());
}

TEST(SolidSyslogServiceEventReport, TheReportsMsgSaysItIsASolidSyslogEvent)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL("SolidSyslog event", LastMsg().c_str());
}

TEST(SolidSyslogServiceEventReport, TheLibrarysElementFollowsTheLoggersOwnSd)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS("[spy][solidsyslog@67478", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, TheElementNamesTheEventsSource)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"([solidsyslog@67478 source="TestSource")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, TheElementNamesTheEventsCategoryAfterItsSource)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(source="TestSource" category="BUFFER_RECORDS_LOST")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

static const char* SevenIsNamed(int32_t detail)
{
    return (detail == 7) ? "SEVEN" : nullptr;
}

TEST(SolidSyslogServiceEventReport, TheElementNamesTheDetailAfterTheCategory)
{
    static const struct SolidSyslogErrorSource source = {"TestSource", SevenIsNamed};
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &source, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, TEST_DETAIL);

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(category="BUFFER_RECORDS_LOST" detail="SEVEN")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, AnUnnamedDetailIsWrittenAsItsNumber)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(detail="7")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, ANegativeUnnamedDetailKeepsItsSign)
{
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &TEST_SOURCE, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, -3);

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(detail="-3")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, TheElementCarriesTheNativeCodeAfterTheDetail)
{
    SolidSyslog_ErrorWithNativeCode(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &TEST_SOURCE,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        TEST_DETAIL,
        -5
    );

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(detail="7" native="-5")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, AnEventWithNoNativeCodeHasNoNativeParam)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL(
        R"([spy][solidsyslog@67478 source="TestSource" category="BUFFER_RECORDS_LOST" detail="7"])",
        LastField(SYSLOG_FIELD_SDATA).c_str()
    );
}

TEST(SolidSyslogServiceEventReport, ANativeCodeOfZeroIsStillWritten)
{
    SolidSyslog_ErrorWithNativeCode(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &TEST_SOURCE,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        TEST_DETAIL,
        0
    );

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(native="0")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, TheMostNegativeNativeCodeIsWrittenWhole)
{
    SolidSyslog_ErrorWithNativeCode(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &TEST_SOURCE,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        TEST_DETAIL,
        INT32_MIN
    );

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(native="-2147483648")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, AnEventWithNoSourceHasNoSourceParam)
{
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, nullptr, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, TEST_DETAIL);

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL(
        R"([spy][solidsyslog@67478 category="BUFFER_RECORDS_LOST" detail="7"])",
        LastField(SYSLOG_FIELD_SDATA).c_str()
    );
}

TEST(SolidSyslogServiceEventReport, ASourceWithNoNameHasNoSourceParam)
{
    static const struct SolidSyslogErrorSource source = {nullptr, nullptr};
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &source, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, TEST_DETAIL);

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL(
        R"([spy][solidsyslog@67478 category="BUFFER_RECORDS_LOST" detail="7"])",
        LastField(SYSLOG_FIELD_SDATA).c_str()
    );
}

TEST(SolidSyslogServiceEventReport, NothingIsSentWhenNothingIsPending)
{
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);
}

TEST(SolidSyslogServiceEventReport, SeveralRaisesOfOneKindAreReportedOnce)
{
    RaiseBufferLoss();
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}

TEST(SolidSyslogServiceEventReport, TheFallbackLoggerLeavesTheEventForALoggerThatCanReportIt)
{
    struct SolidSyslog* fallback = SolidSyslog_Create(nullptr);
    RaiseBufferLoss();

    SolidSyslog_Service(fallback);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}

TEST(SolidSyslogServiceEventReport, AnEventRaisedBeforeTheLoggerIsCreatedIsReportedByItsFirstService)
{
    SolidSyslog_Destroy(solidSyslog);
    RaiseBufferLoss();
    solidSyslog = SolidSyslog_Create(&config);

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL("BUFFER_RECORDS_LOST", LastField(SYSLOG_FIELD_MSGID).c_str());
}

TEST(SolidSyslogServiceEventReport, ARefusedReportBlocksThePass)
{
    RaiseBufferLoss();
    SenderFake_FailNextSend(fakeSender);

    LONGS_EQUAL(SOLIDSYSLOG_SERVICE_BLOCKED, SolidSyslog_Service(solidSyslog));
}

TEST(SolidSyslogServiceEventReport, ARefusedReportIsSentOnceTheSenderRecoversWhileLogsGoStraightThrough)
{
    RaiseBufferLoss();
    SenderFake_FailNextSend(fakeSender);
    SolidSyslog_Service(solidSyslog);
    struct SolidSyslogMessage message = {SOLIDSYSLOG_FACILITY_USER, SOLIDSYSLOG_SEVERITY_INFORMATIONAL, "LOGGED", "x"};
    SolidSyslog_Log(solidSyslog, &message);

    SolidSyslog_Service(solidSyslog);

    /* The refused report, the logged record, then the report again. */
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, THRICE);
    STRCMP_EQUAL("BUFFER_RECORDS_LOST", LastField(SYSLOG_FIELD_MSGID).c_str());
}

/* A circular buffer and a store that keeps nothing, over a stream sender
 * whose fake keeps every frame it accepted, so the order a pass sends in can
 * be read back. */
// clang-format off
TEST_GROUP(SolidSyslogServiceEventReportOrder)
{
    struct SolidSyslogStream* stream = nullptr;
    struct SolidSyslogSender* sender = nullptr;
    struct SolidSyslogBuffer* ring = nullptr;
    struct SolidSyslog* solidSyslog = nullptr;

    void setup() override
    {
        static uint8_t ringBytes[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(4)];
        ResolverFake_Reset();
        stream = StreamFake_Create();
        struct SolidSyslogStreamSenderConfig senderConfig = {};
        senderConfig.Resolver = ResolverFake_Get();
        senderConfig.Stream = stream;
        senderConfig.Address = AddressFake_Get();
        sender = SolidSyslogStreamSender_Create(&senderConfig);
        ring = SolidSyslogCircularBuffer_Create(SolidSyslogNullMutex_Get(), ringBytes, sizeof(ringBytes), nullptr, nullptr);
        struct SolidSyslogConfig config = {};
        config.Buffer = ring;
        config.Sender = sender;
        config.Store = SolidSyslogNullStore_Get();
        solidSyslog = SolidSyslog_Create(&config);
    }

    void teardown() override
    {
        SolidSyslog_Destroy(solidSyslog);
        SolidSyslogCircularBuffer_Destroy(ring);
        SolidSyslogStreamSender_Destroy(sender);
        StreamFake_Destroy(stream);
    }

    [[nodiscard]] std::string FrameMsgId(size_t index) const
    {
        return SyslogField(StreamFake_Frame(stream, index), SYSLOG_FIELD_MSGID);
    }
};

// clang-format on

TEST(SolidSyslogServiceEventReportOrder, AReportGoesAfterTheHeldRecordAndBeforeTheBuffer)
{
    SolidSyslogBuffer_Write(ring, "first", 5);
    StreamFake_SetSendFails(stream, true);
    SolidSyslog_Service(solidSyslog);
    StreamFake_SetSendFails(stream, false);
    SolidSyslogBuffer_Write(ring, "second", 6);
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(3, StreamFake_FrameCount(stream));
    STRCMP_EQUAL("first", StreamFake_Frame(stream, 0));
    STRCMP_EQUAL("BUFFER_RECORDS_LOST", FrameMsgId(1).c_str());
    STRCMP_EQUAL("second", StreamFake_Frame(stream, 2));
}

static enum SolidSyslogDiscardPolicy PolicyOf(void* context)
{
    return *static_cast<enum SolidSyslogDiscardPolicy*>(context);
}

/* A store that keeps what it is given, unless told to refuse the next write,
 * under a discard policy each test sets. */
// clang-format off
TEST_GROUP(SolidSyslogServiceEventReportWithAStore)
{
    struct SolidSyslogSender* fakeSender = nullptr;
    struct SolidSyslogStore* store = nullptr;
    struct SolidSyslog* solidSyslog = nullptr;
    enum SolidSyslogDiscardPolicy policy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;

    void setup() override
    {
        fakeSender = SenderFake_Create();
        store = StoreFake_Create();
        struct SolidSyslogConfig config = {};
        config.Buffer = SolidSyslogNullBuffer_Get();
        config.Sender = fakeSender;
        config.Store = store;
        config.GetDiscardPolicy = PolicyOf;
        config.DiscardPolicyContext = &policy;
        solidSyslog = SolidSyslog_Create(&config);
    }

    void teardown() override
    {
        SolidSyslog_Destroy(solidSyslog);
        StoreFake_Destroy();
        SenderFake_Destroy(fakeSender);
    }

    /* The report of a first loss is refused under Halt, which halts the
     * logger; a second loss is then pending. */
    void HaltOnARefusedReport()
    {
        policy = SOLIDSYSLOG_DISCARD_POLICY_HALT;
        RaiseBufferLoss();
        StoreFake_FailNextWrite();
        SolidSyslog_Service(solidSyslog);
        RaiseBufferLoss();
    }
};

// clang-format on

TEST(SolidSyslogServiceEventReportWithAStore, TheStoreKeepsTheReport)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(1, StoreFake_WriteCallCount(store));
}

TEST(SolidSyslogServiceEventReportWithAStore, AHaltedLoggerMakesNoReport)
{
    HaltOnARefusedReport();

    LONGS_EQUAL(SOLIDSYSLOG_SERVICE_HALTED, SolidSyslog_Service(solidSyslog));

    LONGS_EQUAL(0, StoreFake_WriteCallCount(store));
}

TEST(SolidSyslogServiceEventReportWithAStore, ThePendingReportIsMadeOnceTheHaltLifts)
{
    HaltOnARefusedReport();
    policy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;

    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(1, StoreFake_WriteCallCount(store));
}
