#include "ScenarioBench.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogSenderCategories.h"
#include "SolidSyslogStoreCategories.h"
#include "SolidSyslogServiceStatus.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(ScenarioBench)
{
    ScenarioBenchOptions options = {4U, 512U, 3U, SOLIDSYSLOG_DISCARD_POLICY_OLDEST};
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
    for (unsigned i = 0U; i < 200U; i++)
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
    for (unsigned i = 0U; (i < 200U) && (status != SOLIDSYSLOG_SERVICE_HALTED); i++)
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
