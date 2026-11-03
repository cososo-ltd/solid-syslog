#include "ScenarioBench.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogSenderCategories.h"
#include "SolidSyslogStoreCategories.h"
#include "SolidSyslogServiceStatus.h"
#include "SolidSyslogTunables.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(ScenarioBench)
{
    /* A block that holds a worst-case record whatever the tunables say. */
    ScenarioBenchOptions options = {4U, static_cast<size_t>(2U * SOLIDSYSLOG_MAX_MESSAGE_SIZE), 3U, SOLIDSYSLOG_DISCARD_POLICY_OLDEST};
};

// clang-format on

TEST(ScenarioBench, ALoggedRecordIsDeliveredByTheNextPass)
{
    ScenarioBench bench(options);
    LONGS_EQUAL(1, bench.Log());
    bench.Service();
    LONGS_EQUAL(1, bench.Delivered().size());
    LONGS_EQUAL(1, bench.Delivered()[0].Ordinal);
    LONGS_EQUAL(1, bench.Delivered()[0].SequenceId);
}

TEST(ScenarioBench, ADownSenderHoldsTheRecordAndIsReportedOnce)
{
    ScenarioBench bench(options);
    bench.SetSenderDown(true);
    bench.Log();
    bench.Service();
    bench.Service();
    LONGS_EQUAL(0, bench.Delivered().size());
    LONGS_EQUAL(1, bench.Events().size());
    LONGS_EQUAL(SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED, bench.Events()[0].Category);
}

TEST(ScenarioBench, ARecoveredSenderDeliversTheHeldRecordAndIsReportedRestored)
{
    ScenarioBench bench(options);
    bench.SetSenderDown(true);
    bench.Log();
    bench.Service();
    bench.SetSenderDown(false);
    bench.Service();
    LONGS_EQUAL(1, bench.Delivered().size());
    LONGS_EQUAL(2, bench.Delivered()[0].Pass);
    LONGS_EQUAL(2, bench.Events().size());
    LONGS_EQUAL(SOLIDSYSLOG_CAT_SENDER_DELIVERY_RESTORED, bench.Events()[1].Category);
}

TEST(ScenarioBench, AFailedDeviceFreezesTheStoreAtTheNextWrite)
{
    ScenarioBench bench(options);
    bench.FailDevice();
    bench.Log();
    bench.Service();
    CHECK_TRUE(bench.StoreIsFrozen());
    LONGS_EQUAL(1, bench.Events().size());
    LONGS_EQUAL(SOLIDSYSLOG_CAT_STORE_FROZEN, bench.Events()[0].Category);
    LONGS_EQUAL(1, bench.Delivered().size());
}

TEST(ScenarioBench, AFullBufferUnderNewestRefusesAndIsReportedOnceFromTheLog)
{
    options.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
    ScenarioBench bench(options);
    /* The records are far smaller than the worst case the ring is sized
     * for, so it takes many more than BufferMessages to fill it. */
    for (unsigned i = 0U; i < (SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(4U) / 32U) + 20U; i++)
    {
        bench.Log();
    }
    LONGS_EQUAL(1, bench.Events().size());
    LONGS_EQUAL(SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, bench.Events()[0].Category);
    LONGS_EQUAL(0, bench.Events()[0].Pass);
}

TEST(ScenarioBench, AServicePassRecordsItsDeviceWorkAndSends)
{
    ScenarioBench bench(options);
    bench.Log();
    bench.Service();
    CHECK_TRUE(bench.DeviceOperationsInLastPass() > 0U);
    LONGS_EQUAL(2, bench.SendsInLastPass());
    LONGS_EQUAL(bench.DeviceOperationsInLastPass(), bench.MaxDeviceOperationsInOnePass());
}

TEST(ScenarioBench, AFullStoreUnderHaltStopsTheLoggerUntilThePolicyChanges)
{
    options.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_HALT;
    ScenarioBench bench(options);
    bench.SetSenderDown(true);
    enum SolidSyslogServiceStatus status = SOLIDSYSLOG_SERVICE_IDLE;
    /* Enough records to fill the store with room to spare. */
    unsigned fill = static_cast<unsigned>((options.MaxBlocks * options.BlockSize) / 32U) + 20U;
    for (unsigned i = 0U; (i < fill) && (status != SOLIDSYSLOG_SERVICE_HALTED); i++)
    {
        bench.Log();
        status = bench.Service();
    }
    LONGS_EQUAL(SOLIDSYSLOG_SERVICE_HALTED, status);
    LONGS_EQUAL(1, bench.Halts());
    LONGS_EQUAL(SOLIDSYSLOG_SERVICE_HALTED, bench.Service());
    bench.SetDiscardPolicy(SOLIDSYSLOG_DISCARD_POLICY_OLDEST);
    CHECK_TRUE(bench.Service() != SOLIDSYSLOG_SERVICE_HALTED);
}

TEST(ScenarioBench, TheRingIsSizedForTheWorstCaseRecords)
{
    ScenarioBench bench(options);
    LONGS_EQUAL(SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(4U), bench.RingBytes());
}
