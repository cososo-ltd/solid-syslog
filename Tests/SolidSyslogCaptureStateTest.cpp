/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stddef.h>
#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "ErrorHandlerFake.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogCaptureState.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogServiceStatus.h"
#include "SolidSyslogStore.h"
#include "StoreFake.h"
#include "TestUtils.h"

using namespace CososoTesting;

struct SolidSyslogSender;
struct SolidSyslogBuffer;
struct SolidSyslogStore;

struct Device
{
    enum SolidSyslogCaptureState Capture;
};

static enum SolidSyslogCaptureState GetCapture(void* context)
{
    auto* device = static_cast<struct Device*>(context);
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
    struct SolidSyslog*       solidSyslog    = nullptr;
    struct SolidSyslogSender* fakeSender     = nullptr;
    struct SolidSyslogBuffer* circularBuffer = nullptr;
    struct Device device = {SOLIDSYSLOG_CAPTURE_ON};

    void CreateOver(struct SolidSyslogStore* store)
    {
        static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(4)];

        fakeSender     = SenderFake_Create();
        circularBuffer = SolidSyslogCircularBuffer_Create(
            SolidSyslogNullMutex_Get(), bufferRing, sizeof(bufferRing), nullptr, nullptr);
        SolidSyslogConfig config = {};
        config.Buffer              = circularBuffer;
        config.Sender              = fakeSender;
        config.Store               = store;
        config.GetCaptureState     = GetCapture;
        config.CaptureStateContext = &device;
        solidSyslog = SolidSyslog_Create(&config);
    }

    void DestroyLogger() const
    {
        SolidSyslog_Destroy(solidSyslog);
        SolidSyslogCircularBuffer_Destroy(circularBuffer);
        SenderFake_Destroy(fakeSender);
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
