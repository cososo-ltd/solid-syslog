/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stddef.h>
#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "FileFake.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogServiceStatus.h"
#include "SolidSyslogTunables.h"
#include "StoreFake.h"
#include "TestUtils.h"

using namespace CososoTesting;

class TEST_SolidSyslogServiceHeldRecordFrozenStore_NothingIsLostAcrossAnOutageWithAFrozenStore_Test;
class TEST_SolidSyslogServiceHeldRecord_NothingIsLostAcrossAnOutageOfSeveralPasses_Test;
class TEST_SolidSyslogServiceHeldRecord_ServiceDoesNotReadTheBufferWhileItHoldsARecord_Test;
struct SolidSyslogBlockDevice;
struct SolidSyslogBuffer;
struct SolidSyslogFile;
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

/* After a two-pass outage, the two failed attempts and three deliveries, the
 * last of them the third record, with nothing left in the buffer. */
#define CHECK_ALL_THREE_DELIVERED()                                       \
    {                                                                     \
        CALLED_FAKE_ON(SenderFake_Send, fakeSender, 5);                   \
        STRCMP_EQUAL("third", SenderFake_LastBufferAsString(fakeSender)); \
        CHECK_BUFFER_EMPTY();                                             \
    }

/* Shared fixture - a CircularBuffer feeding a SenderFake, with the store left
 * to each group. Service over a store that keeps nothing sends a buffered
 * record directly, and holds one the sender refuses for the next pass. */
// clang-format off
TEST_BASE(ServiceHeldRecordBase)
{
    struct SolidSyslog*       solidSyslog    = nullptr;
    struct SolidSyslogSender* fakeSender     = nullptr;
    struct SolidSyslogBuffer* circularBuffer = nullptr;

    void setupPipeline()
    {
        static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(4)];

        fakeSender     = SenderFake_Create();
        circularBuffer = SolidSyslogCircularBuffer_Create(
            SolidSyslogNullMutex_Get(), bufferRing, sizeof(bufferRing));
    }

    void teardownPipeline() const
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

    void ServiceWhileTheSenderIsDown() const
    {
        SenderFake_FailNextSend(fakeSender);
        SolidSyslog_Service(solidSyslog);
    }

    /* Three records logged, then two passes with the sender down: the first
     * is held, the other two wait in the buffer. */
    void LogThreeAcrossATwoPassOutage() const
    {
        SolidSyslogBuffer_Write(circularBuffer, "first", 5);
        SolidSyslogBuffer_Write(circularBuffer, "second", 6);
        SolidSyslogBuffer_Write(circularBuffer, "third", 5);
        ServiceWhileTheSenderIsDown();
        ServiceWhileTheSenderIsDown();
    }
};

TEST_GROUP_BASE(SolidSyslogServiceHeldRecord, ServiceHeldRecordBase)
{
    void setup() override
    {
        setupPipeline();
        CreateOver(SolidSyslogNullStore_Get());
    }

    void teardown() override
    {
        teardownPipeline();
    }
};

/* A BlockStore whose device fails on its first open freezes, and from then on
 * keeps nothing - the case S43.01 leaves a device fault in. */
TEST_GROUP_BASE(SolidSyslogServiceHeldRecordFrozenStore, ServiceHeldRecordBase)
{
    struct FileFakeStorage         storage = {};
    struct SolidSyslogFile*        file    = nullptr;
    struct SolidSyslogBlockDevice* device  = nullptr;
    struct SolidSyslogStore*       store   = nullptr;

    void setup() override
    {
        setupPipeline();
        file   = FileFake_Create(&storage);
        device = SolidSyslogFileBlockDevice_Create(
            file, "/tmp/test_store", 2U * static_cast<size_t>(SOLIDSYSLOG_MAX_MESSAGE_SIZE + 5U + SOLIDSYSLOG_MAX_INTEGRITY_SIZE));

        struct SolidSyslogBlockStoreConfig config = {};
        config.BlockDevice   = device;
        config.MaxBlocks     = 2U;
        FileFake_FailNextOpen(file);
        store = SolidSyslogBlockStore_Create(&config);
        CreateOver(store);
    }

    void teardown() override
    {
        teardownPipeline();
        SolidSyslogBlockStore_Destroy(store);
        SolidSyslogFileBlockDevice_Destroy(device);
        FileFake_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogServiceHeldRecord, ServiceReturnsBlockedWhenADirectSendFails)
{
    SolidSyslogBuffer_Write(circularBuffer, "first", 5);
    SenderFake_FailNextSend(fakeSender);

    LONGS_EQUAL(SOLIDSYSLOG_SERVICE_BLOCKED, SolidSyslog_Service(solidSyslog));
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
    LogThreeAcrossATwoPassOutage();

    SolidSyslog_Service(solidSyslog);

    CHECK_ALL_THREE_DELIVERED();
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

TEST(SolidSyslogServiceHeldRecordFrozenStore, NothingIsLostAcrossAnOutageWithAFrozenStore)
{
    LogThreeAcrossATwoPassOutage();

    SolidSyslog_Service(solidSyslog);

    CHECK_ALL_THREE_DELIVERED();
}
