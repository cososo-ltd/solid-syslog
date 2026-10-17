/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stddef.h>
#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogServiceStatus.h"
#include "StoreFake.h"
#include "TestUtils.h"

using namespace CososoTesting;

class TEST_SolidSyslogServiceHeldRecord_NothingIsLostAcrossAnOutageOfSeveralPasses_Test;
class TEST_SolidSyslogServiceHeldRecord_ServiceDoesNotReadTheBufferWhileItHoldsARecord_Test;
struct SolidSyslogBuffer;
struct SolidSyslogSender;
struct SolidSyslogStore;

/* The record at the head of the buffer, read out of it. */
#define CHECK_BUFFER_NEXT(expected, length)                                                  \
    {                                                                                        \
        char next[64];                                                                       \
        size_t nextLength = 0U;                                                              \
        CHECK_TRUE(SolidSyslogBuffer_Read(circularBuffer, next, sizeof(next), &nextLength)); \
        LONGS_EQUAL((length), nextLength);                                                   \
        MEMCMP_EQUAL((expected), next, (length));                                            \
    }

#define CHECK_BUFFER_EMPTY()                                                                  \
    {                                                                                         \
        char next[64];                                                                        \
        size_t nextLength = 0U;                                                               \
        CHECK_FALSE(SolidSyslogBuffer_Read(circularBuffer, next, sizeof(next), &nextLength)); \
    }

/* Service over a store that keeps nothing: a buffered record goes straight to
 * the sender, and one the sender refuses is held by Service for the next pass. */
// clang-format off
TEST_GROUP(SolidSyslogServiceHeldRecord)
{
    struct SolidSyslog*       solidSyslog    = nullptr;
    struct SolidSyslogSender* fakeSender     = nullptr;
    struct SolidSyslogBuffer* circularBuffer = nullptr;

    void setup() override
    {
        static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(4)];

        fakeSender     = SenderFake_Create();
        circularBuffer = SolidSyslogCircularBuffer_Create(
            SolidSyslogNullMutex_Get(), bufferRing, sizeof(bufferRing));
        CreateOver(SolidSyslogNullStore_Get());
    }

    void teardown() override
    {
        SolidSyslog_Destroy(solidSyslog);
        SolidSyslogCircularBuffer_Destroy(circularBuffer);
        SenderFake_Destroy(fakeSender);
    }

    void CreateOver(struct SolidSyslogStore* store)
    {
        SolidSyslogConfig config = {};
        config.Buffer            = circularBuffer;
        config.Sender            = fakeSender;
        config.Store             = store;
        solidSyslog = SolidSyslog_Create(&config);
    }

    enum SolidSyslogServiceStatus ServiceWhileTheSenderIsDown()
    {
        SenderFake_FailNextSend(fakeSender);
        return SolidSyslog_Service(solidSyslog);
    }
};

// clang-format on

TEST(SolidSyslogServiceHeldRecord, ServiceReturnsBlockedWhenADirectSendFails)
{
    SolidSyslogBuffer_Write(circularBuffer, "first", 5);

    LONGS_EQUAL(SOLIDSYSLOG_SERVICE_BLOCKED, ServiceWhileTheSenderIsDown());
}

TEST(SolidSyslogServiceHeldRecord, ServiceDoesNotReadTheBufferWhileItHoldsARecord)
{
    SolidSyslogBuffer_Write(circularBuffer, "first", 5);
    SolidSyslogBuffer_Write(circularBuffer, "second", 6);

    ServiceWhileTheSenderIsDown();

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
    CHECK_BUFFER_NEXT("second", 6);
}

TEST(SolidSyslogServiceHeldRecord, ServiceRetriesTheHeldRecordBeforeReadingTheBuffer)
{
    SolidSyslogBuffer_Write(circularBuffer, "first", 5);
    SolidSyslogBuffer_Write(circularBuffer, "second", 6);
    ServiceWhileTheSenderIsDown();

    ServiceWhileTheSenderIsDown();

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, TWICE);
    STRCMP_EQUAL("first", SenderFake_LastBufferAsString(fakeSender));
}

TEST(SolidSyslogServiceHeldRecord, ServiceDeliversTheHeldRecordThenTheBufferOnceTheSenderRecovers)
{
    SolidSyslogBuffer_Write(circularBuffer, "first", 5);
    SolidSyslogBuffer_Write(circularBuffer, "second", 6);
    ServiceWhileTheSenderIsDown();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, THRICE);
    STRCMP_EQUAL("second", SenderFake_LastBufferAsString(fakeSender));
}

TEST(SolidSyslogServiceHeldRecord, ServiceReturnsIdleOnceTheHeldRecordIsSentAndNothingWaits)
{
    SolidSyslogBuffer_Write(circularBuffer, "first", 5);
    ServiceWhileTheSenderIsDown();

    LONGS_EQUAL(SOLIDSYSLOG_SERVICE_IDLE, SolidSyslog_Service(solidSyslog));
}

TEST(SolidSyslogServiceHeldRecord, NothingIsLostAcrossAnOutageOfSeveralPasses)
{
    SolidSyslogBuffer_Write(circularBuffer, "first", 5);
    SolidSyslogBuffer_Write(circularBuffer, "second", 6);
    SolidSyslogBuffer_Write(circularBuffer, "third", 5);
    ServiceWhileTheSenderIsDown();
    ServiceWhileTheSenderIsDown();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, 5);
    STRCMP_EQUAL("third", SenderFake_LastBufferAsString(fakeSender));
    CHECK_BUFFER_EMPTY();
}

TEST(SolidSyslogServiceHeldRecord, DestroyDropsAHeldRecord)
{
    SolidSyslogBuffer_Write(circularBuffer, "first", 5);
    ServiceWhileTheSenderIsDown();

    SolidSyslog_Destroy(solidSyslog);
    CreateOver(SolidSyslogNullStore_Get());
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}

TEST(SolidSyslogServiceHeldRecord, ServiceDoesNotHoldARecordTheStoreRefused)
{
    struct SolidSyslogStore* fakeStore = StoreFake_Create();
    SolidSyslog_Destroy(solidSyslog);
    CreateOver(fakeStore);
    SolidSyslogBuffer_Write(circularBuffer, "first", 5);
    SolidSyslogBuffer_Write(circularBuffer, "second", 6);
    StoreFake_FailNextWrite();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
    STRCMP_EQUAL("second", SenderFake_LastBufferAsString(fakeSender));
    StoreFake_Destroy();
}
