/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "ErrorHandlerFake.h"
#include "FileFake.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogServiceStatus.h"
#include "SolidSyslogStore.h"
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
            SolidSyslogNullMutex_Get(), bufferRing, sizeof(bufferRing), nullptr, nullptr);
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

static enum SolidSyslogDiscardPolicy AnswersHalt(void* context)
{
    (void) context;
    return SOLIDSYSLOG_DISCARD_POLICY_HALT;
}

static enum SolidSyslogDiscardPolicy AnswersConfiguredPolicy(void* context)
{
    return *static_cast<const enum SolidSyslogDiscardPolicy*>(context);
}

static int bufferFullHaltCount;

/* The ring reports its own evictions under the same category, so Service's
 * reports are told apart by their source. */
static int HeldRecordDropReports()
{
    int count = 0;
    for (int index = 0; index < ErrorHandlerFake_HandleCallCount(); index++)
    {
        if (ErrorHandlerFake_SourceAt(index) == &SolidSyslogErrorSource)
        {
            count++;
        }
    }
    return count;
}

static void CountBufferFullHalt(void* context)
{
    (void) context;
    bufferFullHaltCount++;
}

/* Service holds "first" for a sender that is down while the ring fills behind
 * it, and the one policy, on the logger and on the ring, says what happens to
 * the record Service holds. The 16-byte ring holds "second" (8 bytes framed)
 * and "third" (7), so "forth" (7) finds no room; under Oldest it costs
 * "second" alone, since 7 bytes wrap into the 8 that frees. */
// clang-format off
TEST_GROUP(SolidSyslogServiceHeldRecordBufferFull)
{
    struct SolidSyslog*           solidSyslog = nullptr;
    struct SolidSyslogSender*     fakeSender  = nullptr;
    struct SolidSyslogBuffer*     ring        = nullptr;
    uint8_t                       ringBytes[16] = {};
    enum SolidSyslogDiscardPolicy policy      = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;

    void setup() override
    {
        fakeSender = SenderFake_Create();
        ring       = SolidSyslogCircularBuffer_Create(
            SolidSyslogNullMutex_Get(), ringBytes, sizeof(ringBytes), AnswersConfiguredPolicy, &policy);

        SolidSyslogConfig config    = {};
        config.Buffer               = ring;
        config.Sender               = fakeSender;
        config.Store                = SolidSyslogNullStore_Get();
        config.GetDiscardPolicy     = AnswersConfiguredPolicy;
        config.DiscardPolicyContext = &policy;
        config.OnHalt               = CountBufferFullHalt;
        solidSyslog = SolidSyslog_Create(&config);
        bufferFullHaltCount = 0;
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
        SolidSyslog_Destroy(solidSyslog);
        SolidSyslogCircularBuffer_Destroy(ring);
        SenderFake_Destroy(fakeSender);
    }

    void HoldFirstForADownSender() const
    {
        SolidSyslogBuffer_Write(ring, "first", 5);
        SenderFake_FailNextSend(fakeSender);
        SolidSyslog_Service(solidSyslog);
    }

    /* Two fit; the third finds the ring full, and the ring applies the policy. */
    void FillTheRingBehindIt() const
    {
        SolidSyslogBuffer_Write(ring, "second", 6);
        SolidSyslogBuffer_Write(ring, "third", 5);
        SolidSyslogBuffer_Write(ring, "forth", 5);
    }

    void HoldFirstWithTheRingFullBehindIt() const
    {
        HoldFirstForADownSender();
        FillTheRingBehindIt();
    }
};

// clang-format on

TEST(SolidSyslogServiceHeldRecordBufferFull, UnderOldestTheHeldRecordIsDroppedAndTheBufferHeadTakenInstead)
{
    HoldFirstWithTheRingFullBehindIt();

    SolidSyslog_Service(solidSyslog);

    /* The failed attempt, then "third" and "forth": "first" went, as did
     * "second" when the ring made room. */
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, THRICE);
    STRCMP_EQUAL("forth", SenderFake_LastBufferAsString(fakeSender));
}

TEST(SolidSyslogServiceHeldRecordBufferFull, UnderOldestDroppingTheHeldRecordIsReportedAsRecordsLost)
{
    HoldFirstWithTheRingFullBehindIt();
    ErrorHandlerFake_Install(nullptr);

    SolidSyslog_Service(solidSyslog);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        SOLIDSYSLOG_ERROR_HELD_RECORD_DROPPED
    );
}

TEST(SolidSyslogServiceHeldRecordBufferFull, ARunOfDroppedHeldRecordsIsReportedOnce)
{
    HoldFirstWithTheRingFullBehindIt();
    ErrorHandlerFake_Install(nullptr);
    SenderFake_FailNextSend(fakeSender);
    SolidSyslog_Service(solidSyslog); /* drops "first", holds "third" */
    FillTheRingBehindIt();

    SenderFake_FailNextSend(fakeSender);
    SolidSyslog_Service(solidSyslog); /* drops "third" */

    LONGS_EQUAL(1, HeldRecordDropReports());
}

TEST(SolidSyslogServiceHeldRecordBufferFull, AHeldRecordDealtWithEndsTheRunSoTheNextDropIsReportedAgain)
{
    HoldFirstWithTheRingFullBehindIt();
    ErrorHandlerFake_Install(nullptr);
    SolidSyslog_Service(solidSyslog); /* drops "first", delivers the rest */
    HoldFirstWithTheRingFullBehindIt();

    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(2, HeldRecordDropReports());
}

TEST(SolidSyslogServiceHeldRecordBufferFull, UnderOldestTheHeldRecordIsKeptWhileTheBufferHasRoom)
{
    HoldFirstForADownSender();
    SolidSyslogBuffer_Write(ring, "second", 6);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, THRICE);
    STRCMP_EQUAL("second", SenderFake_LastBufferAsString(fakeSender));
    CALLED_FAKE(ErrorHandlerFake_Handle, 0);
}

TEST(SolidSyslogServiceHeldRecordBufferFull, UnderNewestTheHeldRecordIsKept)
{
    policy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
    HoldFirstWithTheRingFullBehindIt();

    SolidSyslog_Service(solidSyslog);

    /* The failed attempt, then "first", "second" and "third"; "forth" was
     * refused by the ring. */
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, 4);
    STRCMP_EQUAL("third", SenderFake_LastBufferAsString(fakeSender));
}

TEST(SolidSyslogServiceHeldRecordBufferFull, UnderHaltServiceHaltsWithoutSendingTheHeldRecord)
{
    policy = SOLIDSYSLOG_DISCARD_POLICY_HALT;
    HoldFirstWithTheRingFullBehindIt();

    LONGS_EQUAL(SOLIDSYSLOG_SERVICE_HALTED, SolidSyslog_Service(solidSyslog));

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
    LONGS_EQUAL(1, bufferFullHaltCount);
}

TEST(SolidSyslogServiceHeldRecordBufferFull, UnderHaltTheHeldRecordIsRetriedOnceThePolicyChanges)
{
    policy = SOLIDSYSLOG_DISCARD_POLICY_HALT;
    HoldFirstWithTheRingFullBehindIt();
    SolidSyslog_Service(solidSyslog);

    policy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, 4);
    STRCMP_EQUAL("third", SenderFake_LastBufferAsString(fakeSender));
}

static int haltOrder;
static int thresholdOrder;
static int reportsBeforeHalt;
static int nextOrder;

static void NoteHalt(void* context)
{
    (void) context;
    haltOrder = ++nextOrder;
    reportsBeforeHalt = ErrorHandlerFake_HandleCallCount();
}

static void NoteThresholdCrossed(void* context)
{
    (void) context;
    thresholdOrder = ++nextOrder;
}

static size_t WholeStore(void* context)
{
    return *static_cast<const size_t*>(context);
}

/* A block store one record per block and two blocks deep, so the third record
 * Service drains into it is the one its policy refuses. What Service does on
 * that refusal under HALT is what these pin: the order the integrator hears
 * about it in. */
// clang-format off
TEST_GROUP_BASE(SolidSyslogServiceHaltOrdering, ServiceHeldRecordBase)
{
    static const size_t RECORD_OVERHEAD = 5;

    struct FileFakeStorage         storage   = {};
    struct SolidSyslogFile*        file      = nullptr;
    struct SolidSyslogBlockDevice* device    = nullptr;
    struct SolidSyslogStore*       store     = nullptr;
    size_t                         threshold = 0U;
    char                           maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};

    void setup() override
    {
        setupPipeline();
        memset(maxMsg, 'A', sizeof(maxMsg));
        file   = FileFake_Create(&storage);
        device = SolidSyslogFileBlockDevice_Create(file, "/tmp/test_store", SOLIDSYSLOG_MAX_MESSAGE_SIZE + RECORD_OVERHEAD);

        struct SolidSyslogBlockStoreConfig storeConfig = {};
        storeConfig.BlockDevice          = device;
        storeConfig.MaxBlocks            = 2U;
        storeConfig.GetDiscardPolicy     = AnswersHalt;
        storeConfig.GetCapacityThreshold = WholeStore;
        storeConfig.OnThresholdCrossed   = NoteThresholdCrossed;
        storeConfig.ThresholdContext     = &threshold;
        store     = SolidSyslogBlockStore_Create(&storeConfig);
        threshold = SolidSyslogStore_GetTotalBytes(store);

        SolidSyslogConfig config    = {};
        config.Buffer               = circularBuffer;
        config.Sender               = fakeSender;
        config.Store                = store;
        config.GetDiscardPolicy     = AnswersHalt;
        config.OnHalt               = NoteHalt;
        solidSyslog = SolidSyslog_Create(&config);

        nextOrder         = 0;
        haltOrder         = 0;
        thresholdOrder    = 0;
        reportsBeforeHalt = -1;
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
        teardownPipeline();
        SolidSyslogBlockStore_Destroy(store);
        SolidSyslogFileBlockDevice_Destroy(device);
        FileFake_Destroy();
    }

    void LogThreeTheStoreCannotAllHold() const
    {
        SolidSyslogBuffer_Write(circularBuffer, maxMsg, sizeof(maxMsg));
        SolidSyslogBuffer_Write(circularBuffer, maxMsg, sizeof(maxMsg));
        SolidSyslogBuffer_Write(circularBuffer, maxMsg, sizeof(maxMsg));
    }
};

// clang-format on

/* An OnHalt that stops the device never returns, so the loss is reported
 * before it is called. */
TEST(SolidSyslogServiceHaltOrdering, TheRefusalIsReportedBeforeOnHaltIsCalled)
{
    LogThreeTheStoreCannotAllHold();

    SolidSyslog_Service(solidSyslog);

    LONGS_EQUAL(1, reportsBeforeHalt);
}

TEST(SolidSyslogServiceHaltOrdering, TheThresholdCallbackFiresBeforeOnHalt)
{
    LogThreeTheStoreCannotAllHold();

    SolidSyslog_Service(solidSyslog);

    CHECK(thresholdOrder > 0);
    CHECK(haltOrder > thresholdOrder);
}
