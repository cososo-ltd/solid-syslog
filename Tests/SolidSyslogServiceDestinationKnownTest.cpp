/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "AddressFake.h"
#include "CppUTest/TestHarness.h"
#include "ErrorHandlerFake.h"
#include "ResolverFake.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogServiceStatus.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogStreamSender.h"
#include "StoreFake.h"
#include "StreamFake.h"
#include "SyslogFieldParser.h"
#include "TestUtils.h"

using namespace CososoTesting;

struct SolidSyslogSender;
struct SolidSyslogBuffer;
struct SolidSyslogStore;
struct SolidSyslogStream;

struct Gate
{
    bool Known;
    int Asked;
};

static bool IsKnown(void* context)
{
    auto* gate = static_cast<struct Gate*>(context);
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
/* The device as wired: a stream sender over the stream fake, which keeps every
 * frame it accepts, behind a store that keeps records, with the error handler
 * fake listening. The gate starts closed, as at boot. */
TEST_GROUP_BASE(SolidSyslogServiceDestinationKnownEndToEnd, DestinationKnownBase)
{
    struct SolidSyslogStream* stream      = nullptr;
    struct SolidSyslogSender* streamSender = nullptr;
    struct SolidSyslogStore*  fakeStore   = nullptr;

    void setup() override
    {
        ResolverFake_Reset();
        stream = StreamFake_Create();
        struct SolidSyslogStreamSenderConfig senderConfig = {};
        senderConfig.Resolver = ResolverFake_Get();
        senderConfig.Stream   = stream;
        senderConfig.Address  = AddressFake_Get();
        streamSender = SolidSyslogStreamSender_Create(&senderConfig);
        fakeStore    = StoreFake_Create();
        gate.Known   = false;
        CreateOver(fakeStore);
        SenderFake_Destroy(fakeSender);
        fakeSender = nullptr;
        SolidSyslog_Destroy(solidSyslog);
        SolidSyslogConfig config = {};
        config.Buffer                  = circularBuffer;
        config.Sender                  = streamSender;
        config.Store                   = fakeStore;
        config.IsDestinationKnown      = IsKnown;
        config.DestinationKnownContext = &gate;
        solidSyslog = SolidSyslog_Create(&config);
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
        SolidSyslog_Destroy(solidSyslog);
        SolidSyslogCircularBuffer_Destroy(circularBuffer);
        SolidSyslogStreamSender_Destroy(streamSender);
        StreamFake_Destroy(stream);
        StoreFake_Destroy();
    }

    [[nodiscard]] std::string FrameMsgId(size_t index) const
    {
        return SyslogField(StreamFake_Frame(stream, index), SYSLOG_FIELD_MSGID);
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

TEST(SolidSyslogServiceDestinationKnownEndToEnd, WhileTheDestinationIsNotKnownNothingIsResolvedOpenedOrReported)
{
    Log("first");
    Log("second");

    SolidSyslog_Service(solidSyslog);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE(ResolverFake_Resolve, NEVER);
    CALLED_FAKE_ON(StreamFake_Open, stream, NEVER);
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogServiceDestinationKnownEndToEnd, OnceKnownTheRecordsArriveInOrderWithNoDeliveryEventBetween)
{
    Log("first");
    Log("second");
    SolidSyslog_Service(solidSyslog);
    gate.Known = true;

    SolidSyslog_Service(solidSyslog);
    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(2, StreamFake_FrameCount(stream));
    STRCMP_EQUAL("first", FrameMsgId(0).c_str());
    STRCMP_EQUAL("second", FrameMsgId(1).c_str());
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}
