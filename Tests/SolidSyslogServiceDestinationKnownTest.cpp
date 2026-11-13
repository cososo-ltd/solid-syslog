/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPrival.h"
#include "TestUtils.h"

using namespace CososoTesting;

struct SolidSyslogSender;
struct SolidSyslogBuffer;

static bool IsKnown(void* context)
{
    return *(bool*) context;
}

// clang-format off
TEST_GROUP(SolidSyslogServiceDestinationKnown)
{
    struct SolidSyslog*       solidSyslog    = nullptr;
    struct SolidSyslogSender* fakeSender     = nullptr;
    struct SolidSyslogBuffer* circularBuffer = nullptr;
    bool known = true;

    void setup() override
    {
        static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(4)];

        fakeSender     = SenderFake_Create();
        circularBuffer = SolidSyslogCircularBuffer_Create(
            SolidSyslogNullMutex_Get(), bufferRing, sizeof(bufferRing), nullptr, nullptr);
        SolidSyslogConfig config = {};
        config.Buffer                  = circularBuffer;
        config.Sender                  = fakeSender;
        config.Store                   = SolidSyslogNullStore_Get();
        config.IsDestinationKnown      = IsKnown;
        config.DestinationKnownContext = &known;
        solidSyslog = SolidSyslog_Create(&config);
    }

    void teardown() override
    {
        SolidSyslog_Destroy(solidSyslog);
        SolidSyslogCircularBuffer_Destroy(circularBuffer);
        SenderFake_Destroy(fakeSender);
    }

    void Log()
    {
        struct SolidSyslogMessage message = {};
        message.Facility = SOLIDSYSLOG_FACILITY_LOCAL0;
        message.Severity = SOLIDSYSLOG_SEVERITY_INFORMATIONAL;
        message.Msg      = "boot";
        SolidSyslog_Log(solidSyslog, &message);
    }
};
// clang-format on

TEST(SolidSyslogServiceDestinationKnown, NotKnownNothingIsOfferedToTheSender)
{
    known = false;
    Log();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);
}
