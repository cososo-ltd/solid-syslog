/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "AddressFake.h"
#include "CounterFake.h"
#include "CppUTest/TestHarness.h"
#include "ErrorHandlerFake.h"
#include "ResolverFake.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogCaptureState.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogServiceStatus.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogStructuredData.h"
#include "StoreFake.h"
#include "StreamFake.h"
#include "SyslogFieldParser.h"
#include "TestUtils.h"

using namespace CososoTesting;

struct SolidSyslogSender;
struct SolidSyslogBuffer;
struct SolidSyslogStore;
struct SolidSyslogStream;

struct Device
{
    enum SolidSyslogCaptureState Capture;
    int Asked;
};

static enum SolidSyslogCaptureState GetCapture(void* context)
{
    auto* device = static_cast<struct Device*>(context);
    device->Asked++;
    return device->Capture;
}

static const struct SolidSyslogErrorSource TEST_SOURCE = {"TestSource", nullptr};

/* An event of the library's own, waiting to be reported. */
static void RaiseBufferLoss()
{
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, &TEST_SOURCE, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, 7);
}

// clang-format off
/* A circular buffer feeding a sender, with the capture state wired to a Device
 * that starts on; the sender and the store are each group's. */
TEST_BASE(CaptureStateBase)
{
    struct SolidSyslog*               solidSyslog    = nullptr;
    struct SolidSyslogSender*         fakeSender     = nullptr;
    struct SolidSyslogBuffer*         circularBuffer = nullptr;
    struct SolidSyslogAtomicCounter*  counter        = nullptr;
    struct SolidSyslogStructuredData* metaSd         = nullptr;
    struct SolidSyslogMutex*          mutex          = SolidSyslogNullMutex_Get();
    struct Device device = {SOLIDSYSLOG_CAPTURE_ON, 0};
    SolidSyslogCaptureStateFunction getCaptureState = GetCapture;

    void CreateOver(struct SolidSyslogStore* store)
    {
        fakeSender = SenderFake_Create();
        CreateOver(fakeSender, store);
    }

    void CreateOver(struct SolidSyslogSender* sender, struct SolidSyslogStore* store)
    {
        static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(4)];

        circularBuffer = SolidSyslogCircularBuffer_Create(
            mutex, bufferRing, sizeof(bufferRing), nullptr, nullptr);
        counter = CounterFake_Create();
        struct SolidSyslogMetaSdConfig metaConfig = {};
        metaConfig.Counter = counter;
        metaSd = SolidSyslogMetaSd_Create(&metaConfig);
        SolidSyslogConfig config = {};
        config.Buffer              = circularBuffer;
        config.Sender              = sender;
        config.Store               = store;
        config.Sd                  = &metaSd;
        config.SdCount             = 1U;
        config.GetCaptureState     = getCaptureState;
        config.CaptureStateContext = &device;
        solidSyslog = SolidSyslog_Create(&config);
    }

    void DestroyLogger() const
    {
        DestroyLoggerKeepingTheSender();
        SenderFake_Destroy(fakeSender);
    }

    /* The sender is a group's own where it is not the fake. */
    void DestroyLoggerKeepingTheSender() const
    {
        SolidSyslog_Destroy(solidSyslog);
        SolidSyslogMetaSd_Destroy(metaSd);
        CounterFake_Destroy(counter);
        SolidSyslogCircularBuffer_Destroy(circularBuffer);
    }

    void Log(const char* messageId = "boot") const
    {
        struct SolidSyslogMessage message = {};
        message.Facility  = SOLIDSYSLOG_FACILITY_LOCAL0;
        message.Severity  = SOLIDSYSLOG_SEVERITY_INFORMATIONAL;
        message.MessageId = messageId;
        message.Msg       = "boot";
        SolidSyslog_Log(solidSyslog, &message);
    }
};

TEST_GROUP_BASE(SolidSyslogCaptureState, CaptureStateBase)
{
    void setup() override
    {
        CreateOver(SolidSyslogNullStore_Get());
    }

    void teardown() override
    {
        DestroyLogger();
    }
};

/* The same logger with nothing wired to the capture state. */
TEST_GROUP_BASE(SolidSyslogCaptureStateUnwired, CaptureStateBase)
{
    void setup() override
    {
        getCaptureState = nullptr;
        CreateOver(SolidSyslogNullStore_Get());
    }

    void teardown() override
    {
        DestroyLogger();
    }
};

/* The same, over a store that keeps records, with the error handler fake
 * listening. */
TEST_GROUP_BASE(SolidSyslogCaptureStateWithAStore, CaptureStateBase)
{
    struct SolidSyslogStore* fakeStore = nullptr;

    void setup() override
    {
        fakeStore = StoreFake_Create();
        CreateOver(fakeStore);
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
        DestroyLogger();
        StoreFake_Destroy();
    }
};

/* The device as wired: a stream sender over the stream fake, which keeps every
 * frame it accepts, behind a store that keeps records, with the error handler
 * fake listening. The device starts undecided, as at boot. */
TEST_GROUP_BASE(SolidSyslogCaptureStateEndToEnd, CaptureStateBase)
{
    struct SolidSyslogStream* stream       = nullptr;
    struct SolidSyslogSender* streamSender = nullptr;
    struct SolidSyslogStore*  fakeStore    = nullptr;

    void setup() override
    {
        ResolverFake_Reset();
        stream = StreamFake_Create();
        struct SolidSyslogStreamSenderConfig senderConfig = {};
        senderConfig.Resolver = ResolverFake_Get();
        senderConfig.Stream   = stream;
        senderConfig.Address  = AddressFake_Get();
        streamSender   = SolidSyslogStreamSender_Create(&senderConfig);
        fakeStore      = StoreFake_Create();
        device.Capture = SOLIDSYSLOG_CAPTURE_UNDECIDED;
        CreateOver(streamSender, fakeStore);
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
        DestroyLoggerKeepingTheSender();
        SolidSyslogStreamSender_Destroy(streamSender);
        StreamFake_Destroy(stream);
        StoreFake_Destroy();
    }

    [[nodiscard]] std::string FrameMsgId(size_t index) const
    {
        return SyslogField(StreamFake_Frame(stream, index), SYSLOG_FIELD_MSGID);
    }
};

/* The buffer's mutex, standing in for a Log on another task: its next Unlock,
 * once armed, is a device that turns on and logs. The purge's Read releases
 * the mutex before Service asks whether the record it read was logged
 * undecided, so that Log lands between the two. */
static CaptureStateBase* loggingMutexFixture = nullptr;
static bool loggingMutexArmed = false;

static void LoggingMutex_Lock(struct SolidSyslogMutex* self)
{
    (void) self;
}

static void LoggingMutex_Unlock(struct SolidSyslogMutex* self)
{
    (void) self;
    if (loggingMutexArmed)
    {
        loggingMutexArmed = false;
        loggingMutexFixture->device.Capture = SOLIDSYSLOG_CAPTURE_ON;
        loggingMutexFixture->Log("second");
    }
}

static struct SolidSyslogMutex loggingMutex = {LoggingMutex_Lock, LoggingMutex_Unlock};

TEST_GROUP_BASE(SolidSyslogCaptureStateRacingALog, CaptureStateBase)
{
    void setup() override
    {
        mutex = &loggingMutex;
        loggingMutexFixture = this;
        loggingMutexArmed = false;
        CreateOver(SolidSyslogNullStore_Get());
    }

    void teardown() override
    {
        loggingMutexFixture = nullptr;
        DestroyLogger();
    }
};
// clang-format on

TEST(SolidSyslogCaptureState, OnTheRecordIsSent)
{
    Log();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}

TEST(SolidSyslogCaptureState, UndecidedNothingIsOfferedToTheSender)
{
    device.Capture = SOLIDSYSLOG_CAPTURE_UNDECIDED;
    Log();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);
}

TEST(SolidSyslogCaptureState, UndecidedThePassIsBlocked)
{
    device.Capture = SOLIDSYSLOG_CAPTURE_UNDECIDED;
    Log();

    LONGS_EQUAL(SOLIDSYSLOG_SERVICE_BLOCKED, SolidSyslog_Service(solidSyslog));
}

TEST(SolidSyslogCaptureStateWithAStore, UndecidedNothingIsWrittenToTheStore)
{
    device.Capture = SOLIDSYSLOG_CAPTURE_UNDECIDED;
    Log();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(StoreFake_Write, fakeStore, NEVER);
}

TEST(SolidSyslogCaptureStateWithAStore, UndecidedNoReportIsMade)
{
    device.Capture = SOLIDSYSLOG_CAPTURE_UNDECIDED;
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(StoreFake_Write, fakeStore, NEVER);
}

TEST(SolidSyslogCaptureState, OffARecordLoggedIsNotSent)
{
    device.Capture = SOLIDSYSLOG_CAPTURE_OFF;
    Log();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);
}

TEST(SolidSyslogCaptureState, OffARecordLoggedConsumesNoSequenceNumber)
{
    device.Capture = SOLIDSYSLOG_CAPTURE_OFF;
    Log();
    device.Capture = SOLIDSYSLOG_CAPTURE_ON;

    Log();

    LONGS_EQUAL(1, CounterFake_LastHandedOut(counter));
}

TEST(SolidSyslogCaptureStateWithAStore, OffARecordLoggedIsNotReportedAsALoss)
{
    device.Capture = SOLIDSYSLOG_CAPTURE_OFF;
    Log();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogCaptureStateWithAStore, OffAfterUndecidedTheRecordsHeldAreDiscarded)
{
    device.Capture = SOLIDSYSLOG_CAPTURE_UNDECIDED;
    Log();
    Log();
    device.Capture = SOLIDSYSLOG_CAPTURE_OFF;

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(StoreFake_Write, fakeStore, NEVER);
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);
}

TEST(SolidSyslogCaptureState, OffAfterOnTheDeactivationRecordIsStillSent)
{
    Log("deactivated");
    device.Capture = SOLIDSYSLOG_CAPTURE_OFF;

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
    STRCMP_CONTAINS("deactivated", SenderFake_LastBufferAsString(fakeSender));
}

TEST(SolidSyslogCaptureState, OnceServiceHasSeenOnARecordLoggedUndecidedLaterIsKeptWhenOff)
{
    device.Capture = SOLIDSYSLOG_CAPTURE_UNDECIDED;
    Log("first");
    device.Capture = SOLIDSYSLOG_CAPTURE_ON;
    SolidSyslog_Service(solidSyslog);
    device.Capture = SOLIDSYSLOG_CAPTURE_UNDECIDED;
    Log("second");
    device.Capture = SOLIDSYSLOG_CAPTURE_OFF;

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, TWICE);
    STRCMP_CONTAINS("second", SenderFake_LastBufferAsString(fakeSender));
}

TEST(SolidSyslogCaptureStateRacingALog, ARecordReadBeforeALogSawOnIsKeptWithIt)
{
    device.Capture = SOLIDSYSLOG_CAPTURE_UNDECIDED;
    Log("first");
    device.Capture = SOLIDSYSLOG_CAPTURE_OFF;
    loggingMutexArmed = true;

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, TWICE);
}

TEST(SolidSyslogCaptureStateUnwired, NothingWiredMeansOn)
{
    Log();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}

TEST(SolidSyslogCaptureState, TheDeviceIsAskedOnceALogAndOnceAPass)
{
    Log();

    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(2, device.Asked);
}

TEST(SolidSyslogCaptureStateEndToEnd, ABootThatFindsLoggingOffLeavesNoTrace)
{
    Log("first");
    Log("second");
    SolidSyslog_Service(solidSyslog);
    device.Capture = SOLIDSYSLOG_CAPTURE_OFF;

    SolidSyslog_Service(solidSyslog);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(StoreFake_Write, fakeStore, NEVER);
    CALLED_FAKE(ResolverFake_Resolve, NEVER);
    CALLED_FAKE_ON(StreamFake_Open, stream, NEVER);
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogCaptureStateEndToEnd, ABootThatFindsLoggingOnDeliversWhatItHeldInOrder)
{
    Log("first");
    Log("second");
    SolidSyslog_Service(solidSyslog);
    device.Capture = SOLIDSYSLOG_CAPTURE_ON;

    SolidSyslog_Service(solidSyslog);
    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(2, StreamFake_FrameCount(stream));
    STRCMP_EQUAL("first", FrameMsgId(0).c_str());
    STRCMP_EQUAL("second", FrameMsgId(1).c_str());
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogCaptureStateEndToEnd, TurnedOffThenOnAgainOnlyTheDeactivationAndReactivationAreDelivered)
{
    device.Capture = SOLIDSYSLOG_CAPTURE_ON;
    Log("deactivated");
    device.Capture = SOLIDSYSLOG_CAPTURE_OFF;
    Log("while-off");
    SolidSyslog_Service(solidSyslog);
    device.Capture = SOLIDSYSLOG_CAPTURE_ON;
    Log("reactivated");

    SolidSyslog_Service(solidSyslog);
    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(2, StreamFake_FrameCount(stream));
    STRCMP_EQUAL("deactivated", FrameMsgId(0).c_str());
    STRCMP_EQUAL("reactivated", FrameMsgId(1).c_str());
    STRCMP_CONTAINS("sequenceId=\"2\"", StreamFake_Frame(stream, 1));
}
