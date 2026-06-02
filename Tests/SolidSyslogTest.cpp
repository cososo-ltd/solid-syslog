#include <stdint.h>
#include <cstring>
#include <string>

#include "BufferFake.h"
#include "CppUTest/TestHarness.h"
#include "ErrorHandlerFake.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogAtomicCounterTestHelper.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPassthroughBuffer.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogStructuredDataDefinition.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogTunables.h"
#include "StoreFake.h"
#include "StringFake.h"
#include "SyslogFieldParser.h"
#include "TestUtils.h"

using namespace CososoTesting;

class TEST_SolidSyslogTimestamp_NullClockProducesNilvalue_Test;
class TEST_SolidSyslogTimestamp_TimestampAppearsInCorrectMessageFieldPosition_Test;
class TEST_SolidSyslog_LogAfterDestroyAndRecreateWithNullFunctionsProducesNilvalues_Test;
class TEST_SolidSyslog_NullGetAppNameProducesNilvalue_Test;
class TEST_SolidSyslog_NullGetHostnameProducesNilvalue_Test;
class TEST_SolidSyslog_NullGetProcessIdProducesNilvalue_Test;
struct SolidSyslogAtomicCounter;
struct SolidSyslogBuffer;
struct SolidSyslogSender;
struct SolidSyslogStore;

// clang-format off
static const char * const TEST_SDATA     = "-";
static const char * const TEST_MSG       = "hello world";
// clang-format on

#define CHECK_TIMESTAMP(expected) STRCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP).c_str())

#define CHECK_TIMESTAMP_IS_NILVALUE() STRCMP_EQUAL("-", SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP).c_str())

#define CHECK_HOSTNAME(expected) STRCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_HOSTNAME).c_str())

#define CHECK_APP_NAME(expected) STRCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_APP_NAME).c_str())

#define CHECK_PROCID(expected) STRCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_PROCID).c_str())

static const char SD_SPY_TEXT[] = "[spy]";
static const char SD_SPY2_TEXT[] = "[spy2]";

static void SdSpyFormat(struct SolidSyslogStructuredData* /* self */, struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_BoundedString(formatter, SD_SPY_TEXT, sizeof(SD_SPY_TEXT) - 1);
}

static struct SolidSyslogStructuredData sdSpy = {SdSpyFormat};

static void SdSpyFormat2(struct SolidSyslogStructuredData* /* self */, struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_BoundedString(formatter, SD_SPY2_TEXT, sizeof(SD_SPY2_TEXT) - 1);
}

static struct SolidSyslogStructuredData sdSpy2 = {SdSpyFormat2};

static void SdFailFormat(struct SolidSyslogStructuredData* /* self */, struct SolidSyslogFormatter* /* formatter */)
{
}

static struct SolidSyslogStructuredData sdFail = {SdFailFormat};

static void IntegrationGetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    *timeQuality = {true, true, SOLIDSYSLOG_SYNC_ACCURACY_OMIT};
}

// clang-format off
TEST_GROUP(SolidSyslog)
{
    SolidSyslogConfig config;
    SolidSyslogMessage message;
    struct SolidSyslog *solidSyslog;
    SolidSyslogBuffer *buffer;
    SolidSyslogStore  *store;
    struct SolidSyslogSender *fakeSender;
    /* Pool-backed handles owned by tests that exercise Meta/TimeQuality SD.
       Held as fixture state so teardown releases their pool slots even if a
       test body fails mid-assertion — otherwise the leaked slot returns the
       fallback to subsequent tests and cascades the failure. */
    struct SolidSyslogAtomicCounter   *metaSdCounter;
    struct SolidSyslogStructuredData  *metaSd;
    struct SolidSyslogStructuredData  *timeQualitySd;

    void setup() override
    {
        fakeSender = SenderFake_Create();
        StringFake_Reset();
        buffer = SolidSyslogPassthroughBuffer_Create(fakeSender);
        store  = SolidSyslogNullStore_Get();
        metaSdCounter = nullptr;
        metaSd        = nullptr;
        timeQualitySd = nullptr;
        config = {buffer, nullptr, nullptr, StringFake_GetHostname, StringFake_GetAppName, StringFake_GetProcessId, store, nullptr, 0};
        solidSyslog = SolidSyslog_Create(&config);
        message = {SOLIDSYSLOG_FACILITY_LOCAL0, SOLIDSYSLOG_SEVERITY_INFORMATIONAL, nullptr, nullptr};
    }

    void teardown() override
    {
        SolidSyslog_Destroy(solidSyslog);
        if (timeQualitySd != nullptr)
        {
            SolidSyslogTimeQualitySd_Destroy(timeQualitySd);
        }
        if (metaSd != nullptr)
        {
            SolidSyslogMetaSd_Destroy(metaSd);
        }
        if (metaSdCounter != nullptr)
        {
            TestAtomicCounter_Destroy(metaSdCounter);
        }
        SolidSyslogPassthroughBuffer_Destroy(buffer);
        SenderFake_Destroy(fakeSender);
    }

    void Log()
    {
        SolidSyslog_Log(solidSyslog, &message);
    }

    [[nodiscard]] const char *lastMessage() const
    {
        return SenderFake_LastBufferAsString(fakeSender);
    }
};

// clang-format on

TEST(SolidSyslog, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslog, NoMessagesAreSentWhenLogIsNotCalled)
{
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);
}

TEST(SolidSyslog, SingleLogCallResultsInOneSend)
{
    Log();
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}

TEST(SolidSyslog, VersionIs1)
{
    Log();
    std::string header = SyslogField(lastMessage(), SYSLOG_FIELD_HEADER);
    auto closingBracket = header.find('>');
    BYTES_EQUAL('1', header.at(closingBracket + 1));
}

TEST(SolidSyslog, NullGetHostnameProducesNilvalue)
{
    config.GetHostname = nullptr;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    CHECK_HOSTNAME("-");
}

TEST(SolidSyslog, NullGetAppNameProducesNilvalue)
{
    config.GetAppName = nullptr;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    CHECK_APP_NAME("-");
}

TEST(SolidSyslog, NullGetProcessIdProducesNilvalue)
{
    config.GetProcessId = nullptr;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    CHECK_PROCID("-");
}

TEST(SolidSyslog, StructuredDataIsNilValue)
{
    Log();
    STRCMP_EQUAL(TEST_SDATA, SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, NullSdArrayWithNonZeroCountDegradesToNilvalueWithoutCrashing)
{
    config.Sd = nullptr;
    config.SdCount = 1;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("-", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, InjectedSdObjectFormatIsCalledDuringLog)
{
    SolidSyslogStructuredData* sdList[] = {&sdSpy};
    config.Sd = sdList;
    config.SdCount = 1;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("[spy]", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, MetaSdProducesSequenceIdInStructuredData)
{
    metaSdCounter = TestAtomicCounter_Create();
    SolidSyslogMetaSdConfig metaConfig{};
    metaConfig.Counter = metaSdCounter;
    metaSd = SolidSyslogMetaSd_Create(&metaConfig);
    SolidSyslogStructuredData* sdList[] = {metaSd};
    config.Sd = sdList;
    config.SdCount = 1;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("[meta sequenceId=\"1\"]", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, MetaSdSequenceIdIncrementsAcrossLogCalls)
{
    metaSdCounter = TestAtomicCounter_Create();
    SolidSyslogMetaSdConfig metaConfig{};
    metaConfig.Counter = metaSdCounter;
    metaSd = SolidSyslogMetaSd_Create(&metaConfig);
    SolidSyslogStructuredData* sdList[] = {metaSd};
    config.Sd = sdList;
    config.SdCount = 1;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    Log();
    STRCMP_EQUAL("[meta sequenceId=\"2\"]", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, MsgFieldPreservedWithMetaSd)
{
    metaSdCounter = TestAtomicCounter_Create();
    SolidSyslogMetaSdConfig metaConfig{};
    metaConfig.Counter = metaSdCounter;
    metaSd = SolidSyslogMetaSd_Create(&metaConfig);
    SolidSyslogStructuredData* sdList[] = {metaSd};
    config.Sd = sdList;
    config.SdCount = 1;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    message.Msg = "hello world";
    Log();
    STRCMP_EQUAL("hello world", SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MultipleSdItemsAreConcatenated)
{
    SolidSyslogStructuredData* sdList[] = {&sdSpy, &sdSpy2};
    config.Sd = sdList;
    config.SdCount = 2;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("[spy][spy2]", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, SingleSdReturningZeroBytesProducesNilvalue)
{
    SolidSyslogStructuredData* sdList[] = {&sdFail};
    config.Sd = sdList;
    config.SdCount = 1;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("-", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, FailingSdIsSkippedWhenOtherSdSucceeds)
{
    SolidSyslogStructuredData* sdList[] = {&sdFail, &sdSpy};
    config.Sd = sdList;
    config.SdCount = 2;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("[spy]", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, AllSdFailingProducesNilvalue)
{
    SolidSyslogStructuredData* sdList[] = {&sdFail, &sdFail};
    config.Sd = sdList;
    config.SdCount = 2;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("-", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, MetaSdAndTimeQualitySdCoexistInSdArray)
{
    metaSdCounter = TestAtomicCounter_Create();
    SolidSyslogMetaSdConfig metaConfig{};
    metaConfig.Counter = metaSdCounter;
    metaSd = SolidSyslogMetaSd_Create(&metaConfig);
    timeQualitySd = SolidSyslogTimeQualitySd_Create(IntegrationGetTimeQuality);
    SolidSyslogStructuredData* sdList[] = {metaSd, timeQualitySd};
    config.Sd = sdList;
    config.SdCount = 2;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL(
        "[meta sequenceId=\"1\"][timeQuality tzKnown=\"1\" isSynced=\"1\"]",
        SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str()
    );
}

TEST(SolidSyslog, NullMessageOmitsMsgField)
{
    Log();
    CHECK_FALSE(SyslogMsgHasBom(lastMessage()));
    STRCMP_EQUAL("", SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MessageBodyAppearsInMessage)
{
    message.Msg = "system started";
    Log();
    STRCMP_EQUAL("system started", SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MessageBodyIsPrecededByUtf8Bom)
{
    message.Msg = "system started";
    Log();
    CHECK(SyslogMsgHasBom(lastMessage()));
}

TEST(SolidSyslog, CallerSuppliedBomIsStrippedSoOutputHasOnlyOne)
{
    message.Msg = "\xEF\xBB\xBFsystem started";
    Log();
    STRCMP_EQUAL("system started", SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, EmptyMessageOmitsMsgField)
{
    message.Msg = "";
    Log();
    CHECK_FALSE(SyslogMsgHasBom(lastMessage()));
    STRCMP_EQUAL("", SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MessageBodyIsNotHardCoded)
{
    message.Msg = TEST_MSG;
    Log();
    STRCMP_EQUAL(TEST_MSG, SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MessageWithSpacesIsPreserved)
{
    message.Msg = "hello world with spaces";
    Log();
    STRCMP_EQUAL("hello world with spaces", SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MessageFillsRemainingBuffer)
{
    std::string header("<134>1 - - - - - - " + std::string(UTF8_BOM));
    size_t maxMsg = SOLIDSYSLOG_MAX_MESSAGE_SIZE - header.size() - 1;
    std::string longMsg(maxMsg, 'X');
    message.Msg = longMsg.c_str();
    Log();
    STRCMP_EQUAL(longMsg.c_str(), SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MessageTruncatedWhenExceedingBuffer)
{
    std::string header("<134>1 - - - - - - " + std::string(UTF8_BOM));
    size_t maxMsg = SOLIDSYSLOG_MAX_MESSAGE_SIZE - header.size() - 1;
    std::string longMsg(maxMsg + 100, 'X');
    message.Msg = longMsg.c_str();
    Log();
    std::string expected(maxMsg, 'X');
    STRCMP_EQUAL(expected.c_str(), SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, BomIsPreservedWhenMessageBodyTruncates)
{
    /* When the body overflows the wire-frame budget, BoundedString clips
     * the body but the BOM — written before the body — must remain
     * present. Pins the FormatMsg ordering: BOM first, body second. */
    std::string longMsg(SOLIDSYSLOG_MAX_MESSAGE_SIZE, 'X');
    message.Msg = longMsg.c_str();
    Log();
    CHECK(SyslogMsgHasBom(lastMessage()));
}

TEST(SolidSyslog, HugeMessageDoesNotCorruptMemory)
{
    std::string hugeMsg(10000, 'Z');
    message.Msg = hugeMsg.c_str();
    Log();
    std::string result = SyslogMsg(lastMessage());
    CHECK(result.size() <= SOLIDSYSLOG_MAX_MESSAGE_SIZE);
}

// clang-format off
static const uint16_t TEST_YEAR        = 2026;
static const uint8_t  TEST_MONTH       = 4;
static const uint8_t  TEST_DAY         = 2;
static const uint8_t  TEST_HOUR        = 14;
static const uint8_t  TEST_MINUTE      = 30;
static const uint8_t  TEST_SECOND      = 7;
static const uint32_t TEST_MICROSECOND = 42;
static const int16_t  TEST_UTC_OFFSET  = 0;
// clang-format on

static struct SolidSyslogTimestamp stubTimestamp;

static void StubClock(struct SolidSyslogTimestamp* timestamp)
{
    *timestamp = stubTimestamp;
}

// clang-format off
TEST_GROUP_BASE(SolidSyslogTimestamp, TEST_GROUP_CppUTestGroupSolidSyslog)
{
    void setup() override
    {
        TEST_GROUP_CppUTestGroupSolidSyslog::setup();
        stubTimestamp = {TEST_YEAR, TEST_MONTH, TEST_DAY, TEST_HOUR, TEST_MINUTE, TEST_SECOND, TEST_MICROSECOND, TEST_UTC_OFFSET};
        config.Clock = StubClock;
        SolidSyslog_Destroy(solidSyslog);
        solidSyslog = SolidSyslog_Create(&config);
    }
};

// clang-format on

TEST(SolidSyslogTimestamp, NullClockProducesNilvalue)
{
    config.Clock = nullptr;
    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
}

TEST(SolidSyslogTimestamp, TimestampAppearsInCorrectMessageFieldPosition)
{
    Log();
    CHECK_TIMESTAMP("2026-04-02T14:30:07.000042Z");
}

TEST(SolidSyslog, ServiceSendsMessageReadFromBuffer)
{
    SolidSyslogBuffer* fakeBuffer = BufferFake_Create();
    SolidSyslogConfig serviceConfig = {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, store, nullptr, 0};

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&serviceConfig);

    SolidSyslogBuffer_Write(fakeBuffer, "test", 4);
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
    STRCMP_EQUAL("test", SenderFake_LastBufferAsString(fakeSender));

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceSendsBufferedMessageWithNullStore)
{
    SolidSyslogBuffer* fakeBuffer = BufferFake_Create();
    SolidSyslogStore* nullStore = SolidSyslogNullStore_Get();
    SolidSyslogConfig serviceConfig =
        {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, nullStore, nullptr, 0};

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&serviceConfig);

    SolidSyslogBuffer_Write(fakeBuffer, "test", 4);
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
    STRCMP_EQUAL("test", SenderFake_LastBufferAsString(fakeSender));

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceSendsFromStoreWhenHasUnsent)
{
    SolidSyslogBuffer* fakeBuffer = BufferFake_Create();
    SolidSyslogStore* fakeStore = StoreFake_Create();
    SolidSyslogConfig serviceConfig =
        {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&serviceConfig);

    SolidSyslogStore_Write(fakeStore, "stored", 6);
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
    STRCMP_EQUAL("stored", SenderFake_LastBufferAsString(fakeSender));

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceMarksSentAfterSuccessfulSend)
{
    SolidSyslogBuffer* fakeBuffer = BufferFake_Create();
    SolidSyslogStore* fakeStore = StoreFake_Create();
    SolidSyslogConfig serviceConfig =
        {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&serviceConfig);

    SolidSyslogStore_Write(fakeStore, "stored", 6);
    SolidSyslog_Service(solidSyslog);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(fakeStore));

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceDoesNotMarkSentOnSendFailure)
{
    SolidSyslogBuffer* fakeBuffer = BufferFake_Create();
    SolidSyslogStore* fakeStore = StoreFake_Create();
    SolidSyslogConfig serviceConfig =
        {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&serviceConfig);

    SolidSyslogStore_Write(fakeStore, "stored", 6);
    SenderFake_FailNextSend(fakeSender);
    SolidSyslog_Service(solidSyslog);

    CHECK_TRUE(SolidSyslogStore_HasUnsent(fakeStore));

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceWritesBufferMessageToStore)
{
    SolidSyslogBuffer* fakeBuffer = BufferFake_Create();
    SolidSyslogStore* fakeStore = StoreFake_Create();
    SolidSyslogConfig serviceConfig =
        {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&serviceConfig);

    SolidSyslogBuffer_Write(fakeBuffer, "buffered", 8);
    SenderFake_FailNextSend(fakeSender);
    SolidSyslog_Service(solidSyslog);

    char readData[512];
    size_t readSize = 0;
    SolidSyslogStore_ReadNextUnsent(fakeStore, readData, sizeof(readData), &readSize);
    LONGS_EQUAL(8, readSize);
    MEMCMP_EQUAL("buffered", readData, 8);

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceSendsStoreMessageNotBufferMessage)
{
    SolidSyslogBuffer* fakeBuffer = BufferFake_Create();
    SolidSyslogStore* fakeStore = StoreFake_Create();
    SolidSyslogConfig serviceConfig =
        {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&serviceConfig);

    SolidSyslogStore_Write(fakeStore, "old", 3);
    SolidSyslogBuffer_Write(fakeBuffer, "new", 3);
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL("old", SenderFake_LastBufferAsString(fakeSender));

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

/* A non-transient store's Write rejection is its discard policy speaking;
 * Service must not bypass to the sender, or a newer message would jump
 * ahead of older retained ones once the sender recovered. (NullStore-side
 * fallthrough is covered by ServiceSendsBufferedMessageWithNullStore.) */
TEST(SolidSyslog, ServiceDoesNotBypassToSenderWhenNonTransientStoreRejectsWrite)
{
    SolidSyslogBuffer* fakeBuffer = BufferFake_Create();
    SolidSyslogStore* fakeStore = StoreFake_Create();
    SolidSyslogConfig serviceConfig =
        {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&serviceConfig);

    SolidSyslogBuffer_Write(fakeBuffer, "direct", 6);
    StoreFake_FailNextWrite();
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceDoesNotSendWhenStoreReadFails)
{
    SolidSyslogBuffer* fakeBuffer = BufferFake_Create();
    SolidSyslogStore* fakeStore = StoreFake_Create();
    SolidSyslogConfig serviceConfig =
        {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&serviceConfig);

    SolidSyslogStore_Write(fakeStore, "stored", 6);
    StoreFake_FailNextRead();
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceDoesNotMarkSentWhenSendingFromBuffer)
{
    SolidSyslogBuffer* fakeBuffer = BufferFake_Create();
    SolidSyslogStore* fakeStore = StoreFake_Create();
    SolidSyslogConfig serviceConfig =
        {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&serviceConfig);

    SolidSyslogStore_Write(fakeStore, "in-store", 8);
    SolidSyslogStore_MarkSent(fakeStore);
    SolidSyslogBuffer_Write(fakeBuffer, "from-buffer", 11);
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
    STRCMP_EQUAL("from-buffer", SenderFake_LastBufferAsString(fakeSender));

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

/* Shared fixture for the eager-drain Service tests — both wire a real
 * CircularBuffer (drives the multi-message-per-tick path) and a FIFO
 * StoreFake. Storage is static so a CHECK failure that skips the test
 * body's cleanup cannot leave a dangling stack reference behind for
 * SolidSyslog_Destroy in teardown. */
// clang-format off
TEST_GROUP(SolidSyslogServiceEagerDrain)
{
    static constexpr size_t BUFFER_BYTES = 256;

    struct SolidSyslog*       solidSyslog    = nullptr;
    struct SolidSyslogSender* fakeSender     = nullptr;
    struct SolidSyslogBuffer* circularBuffer = nullptr;
    struct SolidSyslogStore*  fakeStore      = nullptr;

    void setup() override
    {
        static uint8_t bufferRing[BUFFER_BYTES];

        fakeSender     = SenderFake_Create();
        circularBuffer = SolidSyslogCircularBuffer_Create(
            SolidSyslogNullMutex_Get(), bufferRing, sizeof(bufferRing));
        fakeStore      = StoreFake_Create();

        SolidSyslogConfig serviceConfig = {};
        serviceConfig.Buffer            = circularBuffer;
        serviceConfig.Sender            = fakeSender;
        serviceConfig.Store             = fakeStore;
        solidSyslog = SolidSyslog_Create(&serviceConfig);
    }

    void teardown() override
    {
        SolidSyslog_Destroy(solidSyslog);
        StoreFake_Destroy();
        SolidSyslogCircularBuffer_Destroy(circularBuffer);
        SenderFake_Destroy(fakeSender);
    }
};

// clang-format on

TEST(SolidSyslogServiceEagerDrain, AllBufferedMessagesReachStoreInOneTickWhenSenderFails)
{
    SolidSyslogBuffer_Write(circularBuffer, "msg1", 4);
    SolidSyslogBuffer_Write(circularBuffer, "msg2", 4);
    SolidSyslogBuffer_Write(circularBuffer, "msg3", 4);
    SenderFake_FailNextSend(fakeSender);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(StoreFake_Write, fakeStore, THRICE);
}

TEST(SolidSyslogServiceEagerDrain, StoredMessagesDrainInFifoOrderAcrossTicks)
{
    SolidSyslogBuffer_Write(circularBuffer, "m1", 2);
    SolidSyslogBuffer_Write(circularBuffer, "m2", 2);
    SolidSyslogBuffer_Write(circularBuffer, "m3", 2);

    SolidSyslog_Service(solidSyslog);
    STRCMP_EQUAL("m1", SenderFake_LastBufferAsString(fakeSender));
    SolidSyslog_Service(solidSyslog);
    STRCMP_EQUAL("m2", SenderFake_LastBufferAsString(fakeSender));
    SolidSyslog_Service(solidSyslog);
    STRCMP_EQUAL("m3", SenderFake_LastBufferAsString(fakeSender));
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, THRICE);
}

TEST(SolidSyslog, ServiceDoesNothingWhenStoreIsHalted)
{
    SolidSyslogBuffer* fakeBuffer = BufferFake_Create();
    SolidSyslogStore* fakeStore = StoreFake_Create();
    SolidSyslogConfig serviceConfig =
        {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&serviceConfig);

    SolidSyslogBuffer_Write(fakeBuffer, "test", 4);
    StoreFake_SetHalted();
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);

    SolidSyslog_Destroy(solidSyslog);
    solidSyslog = SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, LogAfterDestroyAndRecreateWithNullFunctionsProducesNilvalues)
{
    SolidSyslog_Destroy(solidSyslog);
    SolidSyslogConfig nilConfig = {buffer, nullptr, nullptr, nullptr, nullptr, nullptr, store, nullptr, 0};
    solidSyslog = SolidSyslog_Create(&nilConfig);
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
    CHECK_HOSTNAME("-");
    CHECK_APP_NAME("-");
    CHECK_PROCID("-");
}

// clang-format off
TEST_GROUP(SolidSyslogLifecycle)
{
    SolidSyslogMessage message{};
    SolidSyslogBuffer* buffer = nullptr;
    SolidSyslogSender* sender = nullptr;
    SolidSyslogStore*  store  = nullptr;
    struct SolidSyslog* solidSyslog = nullptr;

    void setup() override
    {
        solidSyslog = nullptr;
        message = {SOLIDSYSLOG_FACILITY_LOCAL0, SOLIDSYSLOG_SEVERITY_INFORMATIONAL, nullptr, nullptr};
        sender = SenderFake_Create();
        buffer = BufferFake_Create();
        store = SolidSyslogNullStore_Get();
    }

    void teardown() override
    {
        if (solidSyslog != nullptr)
        {
            SolidSyslog_Destroy(solidSyslog);
        }
        BufferFake_Destroy();
        SenderFake_Destroy(sender);
    }

    [[nodiscard]] SolidSyslogConfig validConfig() const
    {
        return {buffer, sender, nullptr, nullptr, nullptr, nullptr, store, nullptr, 0};
    }
};

// clang-format on

TEST(SolidSyslogLifecycle, ServiceWithNullHandleReportsError)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslog_Service(nullptr);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&SolidSyslogErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_ARGUMENT, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_ERROR_SERVICE_NULL_HANDLE, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogLifecycle, LogWithNullHandleReportsError)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslog_Log(nullptr, &message);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&SolidSyslogErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_ARGUMENT, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_ERROR_LOG_NULL_HANDLE, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogLifecycle, LogWithNullMessageReportsError)
{
    SolidSyslogConfig config = validConfig();
    solidSyslog = SolidSyslog_Create(&config);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslog_Log(solidSyslog, nullptr);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&SolidSyslogErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_ARGUMENT, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_ERROR_LOG_NULL_MESSAGE, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogLifecycle, CreateWithNullConfigReportsError)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslog_Create(nullptr);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&SolidSyslogErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_CONFIG, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_ERROR_CREATE_NULL_CONFIG, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogLifecycle, CreateWithNullBufferReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    SolidSyslogConfig config = validConfig();
    config.Buffer = nullptr;

    solidSyslog = SolidSyslog_Create(&config);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&SolidSyslogErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_CONFIG, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_ERROR_CREATE_NULL_BUFFER, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogLifecycle, CreateWithNullSenderReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    SolidSyslogConfig config = validConfig();
    config.Sender = nullptr;

    solidSyslog = SolidSyslog_Create(&config);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&SolidSyslogErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_CONFIG, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_ERROR_CREATE_NULL_SENDER, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogLifecycle, CreateWithNullStoreReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    SolidSyslogConfig config = validConfig();
    config.Store = nullptr;

    solidSyslog = SolidSyslog_Create(&config);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&SolidSyslogErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_CONFIG, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_ERROR_CREATE_NULL_STORE, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogLifecycle, CreateWithNullSdArrayAndNonZeroCountReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    SolidSyslogConfig config = validConfig();
    config.Sd = nullptr;
    config.SdCount = 1;

    solidSyslog = SolidSyslog_Create(&config);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&SolidSyslogErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_CONFIG, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_ERROR_CREATE_INCONSISTENT_SD, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogLifecycle, ServiceWithDefaultStoreDrainsThroughToRealSender)
{
    SolidSyslogConfig config = validConfig();
    config.Store = nullptr;
    solidSyslog = SolidSyslog_Create(&config);
    SolidSyslog_Log(solidSyslog, &message);

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, sender, ONCE);
}

TEST(SolidSyslogLifecycle, DestroyWithUnknownHandleReportsWarning)
{
    /* Any non-pool address is "unknown" to IndexFromHandle. Cast a stack
       byte's address — its value never gets dereferenced, only compared. */
    char stackByte = 0;
    auto* notAHandle = reinterpret_cast<struct SolidSyslog*>(&stackByte);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslog_Destroy(notAHandle);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&SolidSyslogErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogLifecycle, DestroyWithNullHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslog_Destroy(nullptr);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&SolidSyslogErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogLifecycle, CreateWithNullConfigDoesNotBlockSubsequentCreate)
{
    SolidSyslog_Create(nullptr);
    SolidSyslogConfig config = validConfig();

    solidSyslog = SolidSyslog_Create(&config);
    SolidSyslog_Log(solidSyslog, &message);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, sender, ONCE);
}

TEST(SolidSyslogLifecycle, DestroyClearsInitialisedFlagSoCreateSucceedsAgain)
{
    SolidSyslogConfig config = validConfig();
    solidSyslog = SolidSyslog_Create(&config);
    SolidSyslog_Destroy(solidSyslog);
    ErrorHandlerFake_Install(nullptr);

    solidSyslog = SolidSyslog_Create(&config);
    SolidSyslog_Log(solidSyslog, &message);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
    CALLED_FAKE_ON(SenderFake_Send, sender, ONCE);
}
