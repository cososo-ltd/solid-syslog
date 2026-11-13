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
#include "SolidSyslogServiceStatus.h"
#include "SolidSyslogStore.h"
#include "StoreFake.h"
#include "TestUtils.h"

using namespace CososoTesting;

struct SolidSyslogSender;
struct SolidSyslogBuffer;
struct SolidSyslogStore;

struct Gate
{
    bool Known;
    int Asked;
};

static bool IsKnown(void* context)
{
    struct Gate* gate = (struct Gate*) context;
    gate->Asked++;
    return gate->Known;
}

/* A circular buffer feeding a sender fake, with the gate wired to a Gate that
 * starts known; the store is each group's. */
// clang-format off
TEST_BASE(DestinationKnownBase)
{
    struct SolidSyslog*       solidSyslog    = nullptr;
    struct SolidSyslogSender* fakeSender     = nullptr;
    struct SolidSyslogBuffer* circularBuffer = nullptr;
    struct Gate gate = {true, 0};
    SolidSyslogDestinationKnownFunction isDestinationKnown = IsKnown;

    void CreateOver(struct SolidSyslogStore* store)
    {
        static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(4)];

        fakeSender     = SenderFake_Create();
        circularBuffer = SolidSyslogCircularBuffer_Create(
            SolidSyslogNullMutex_Get(), bufferRing, sizeof(bufferRing), nullptr, nullptr);
        SolidSyslogConfig config = {};
        config.Buffer                  = circularBuffer;
        config.Sender                  = fakeSender;
        config.Store                   = store;
        config.IsDestinationKnown      = isDestinationKnown;
        config.DestinationKnownContext = &gate;
        solidSyslog = SolidSyslog_Create(&config);
    }

    void DestroyLogger() const
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

TEST_GROUP_BASE(SolidSyslogServiceDestinationKnown, DestinationKnownBase)
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

/* The same logger with nothing wired to the gate. */
TEST_GROUP_BASE(SolidSyslogServiceDestinationKnownUnwired, DestinationKnownBase)
{
    void setup() override
    {
        isDestinationKnown = nullptr;
        CreateOver(SolidSyslogNullStore_Get());
    }

    void teardown() override
    {
        DestroyLogger();
    }
};

/* The same, over a store that keeps records. */
TEST_GROUP_BASE(SolidSyslogServiceDestinationKnownWithAStore, DestinationKnownBase)
{
    struct SolidSyslogStore* fakeStore = nullptr;

    void setup() override
    {
        fakeStore = StoreFake_Create();
        CreateOver(fakeStore);
    }

    void teardown() override
    {
        DestroyLogger();
        StoreFake_Destroy();
    }
};
// clang-format on

TEST(SolidSyslogServiceDestinationKnown, NotKnownNothingIsOfferedToTheSender)
{
    gate.Known = false;
    Log();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);
}

TEST(SolidSyslogServiceDestinationKnownWithAStore, NotKnownARecordTheStoreKeepsIsNotSent)
{
    gate.Known = false;
    Log();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);
}

TEST(SolidSyslogServiceDestinationKnown, NotKnownThePassIsBlocked)
{
    gate.Known = false;
    Log();

    LONGS_EQUAL(SOLIDSYSLOG_SERVICE_BLOCKED, SolidSyslog_Service(solidSyslog));
}

TEST(SolidSyslogServiceDestinationKnown, KnownAgainTheRecordHeldBackIsSentOnThatPass)
{
    gate.Known = false;
    Log();
    SolidSyslog_Service(solidSyslog);
    gate.Known = true;

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
    STRCMP_CONTAINS("boot", SenderFake_LastBufferAsString(fakeSender));
}

TEST(SolidSyslogServiceDestinationKnown, TheGateIsNotAskedWhenThereIsNothingToSend)
{
    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(0, gate.Asked);
}

TEST(SolidSyslogServiceDestinationKnown, NotKnownTheGateIsAskedOnceAPass)
{
    gate.Known = false;
    Log();
    Log();

    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(1, gate.Asked);
}

TEST(SolidSyslogServiceDestinationKnownUnwired, NothingWiredMeansKnown)
{
    Log();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}

TEST(SolidSyslogServiceDestinationKnownWithAStore, NotKnownTheBufferStillDrainsIntoTheStore)
{
    gate.Known = false;
    Log();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(StoreFake_Write, fakeStore, ONCE);
}

TEST(SolidSyslogServiceDestinationKnownWithAStore, NotKnownAPassWithNothingToDrainIsBlocked)
{
    gate.Known = false;
    Log();
    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(SOLIDSYSLOG_SERVICE_BLOCKED, SolidSyslog_Service(solidSyslog));
}

TEST(SolidSyslogServiceDestinationKnownWithAStore, NotKnownTheGateIsAskedOnceAPass)
{
    gate.Known = false;
    Log();
    Log();

    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(1, gate.Asked);
}

TEST(SolidSyslogServiceDestinationKnownWithAStore, KnownAgainTheStoredRecordsAreSentInOrder)
{
    gate.Known = false;
    Log();
    SolidSyslog_Service(solidSyslog);
    gate.Known = true;

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
    STRCMP_CONTAINS("boot", SenderFake_LastBufferAsString(fakeSender));
}
