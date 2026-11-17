/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stddef.h>
#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogCaptureState.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPrival.h"
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

// clang-format off
TEST_GROUP(SolidSyslogCaptureState)
{
    struct SolidSyslog*       solidSyslog    = nullptr;
    struct SolidSyslogSender* fakeSender     = nullptr;
    struct SolidSyslogBuffer* circularBuffer = nullptr;
    struct Device device = {SOLIDSYSLOG_CAPTURE_ON};

    void setup() override
    {
        static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(4)];

        fakeSender     = SenderFake_Create();
        circularBuffer = SolidSyslogCircularBuffer_Create(
            SolidSyslogNullMutex_Get(), bufferRing, sizeof(bufferRing), nullptr, nullptr);
        SolidSyslogConfig config = {};
        config.Buffer              = circularBuffer;
        config.Sender              = fakeSender;
        config.Store               = SolidSyslogNullStore_Get();
        config.GetCaptureState     = GetCapture;
        config.CaptureStateContext = &device;
        solidSyslog = SolidSyslog_Create(&config);
    }

    void teardown() override
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
// clang-format on

TEST(SolidSyslogCaptureState, OnTheRecordIsSent)
{
    Log();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}
