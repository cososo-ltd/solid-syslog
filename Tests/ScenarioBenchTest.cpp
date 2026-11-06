#include "ScenarioBench.h"
#include "SecurityPolicyFake.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogError.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogSenderCategories.h"
#include "SolidSyslogStoreCategories.h"
#include "SolidSyslogServiceStatus.h"
#include "SolidSyslogTlsStreamCategories.h"
#include "SolidSyslogTunables.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(ScenarioBench)
{
    /* A block that holds a worst-case record whatever the tunables say. */
    ScenarioBenchOptions options = {4U, static_cast<size_t>(2U * SOLIDSYSLOG_MAX_MESSAGE_SIZE), 3U, SOLIDSYSLOG_DISCARD_POLICY_OLDEST, true, false};
};

// clang-format on

/* The records are far smaller than the worst case the ring is sized for, so
 * it takes many more than BufferMessages to fill it. */
static void FillTheRing(ScenarioBench& bench)
{
    for (unsigned i = 0U; i < (SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(4U) / 32U) + 20U; i++)
    {
        bench.Log();
    }
}

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

TEST(ScenarioBench, AnEventCarriesTheRecordsTheStoreHadKeptWhenItWasRaised)
{
    ScenarioBench bench(options);
    bench.SetSenderDown(true);
    bench.Log();
    bench.Log();
    bench.Service();
    LONGS_EQUAL(1, bench.Events().size());
    LONGS_EQUAL(2, bench.Events()[0].RecordsKept);
}

TEST(ScenarioBench, AnEventCarriesTheLastSequenceIdHandedOutWhenItWasRaised)
{
    ScenarioBench bench(options);
    bench.SetSenderDown(true);
    bench.Log();
    bench.Log();
    bench.Service();
    LONGS_EQUAL(1, bench.Events().size());
    LONGS_EQUAL(2, bench.Events()[0].LastSequenceId);
}

TEST(ScenarioBench, AnEventCarriesTheSourceAndDetailParamsAReportOfItWouldCarry)
{
    ScenarioBench bench(options);
    bench.SetSenderDown(true);
    bench.Log();
    bench.Service();
    STRCMP_EQUAL("StreamSender", bench.Events()[0].SourceParam.c_str());
    STRCMP_EQUAL("DELIVERY_FAILED", bench.Events()[0].DetailParam.c_str());
    STRCMP_EQUAL("", bench.Events()[0].NativeParam.c_str());
}

TEST(ScenarioBench, AnEventCarriesAnUnnamedDetailAsItsNumber)
{
    ScenarioBench bench(options);
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, nullptr, SOLIDSYSLOG_CAT_FILE_IO_FAILED, 7);
    STRCMP_EQUAL("", bench.Events()[0].SourceParam.c_str());
    STRCMP_EQUAL("7", bench.Events()[0].DetailParam.c_str());
}

TEST(ScenarioBench, AnEventCarriesItsNativeCode)
{
    ScenarioBench bench(options);
    SolidSyslog_ErrorWithNativeCode(SOLIDSYSLOG_SEVERITY_ERROR, nullptr, SOLIDSYSLOG_CAT_FILE_IO_FAILED, 7, -5);
    STRCMP_EQUAL("-5", bench.Events()[0].NativeParam.c_str());
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

TEST(ScenarioBench, WithoutAStoreARecordGoesStraightToTheSender)
{
    options.HasStore = false;
    ScenarioBench bench(options);
    bench.Log();
    bench.Service();
    LONGS_EQUAL(1, bench.Delivered().size());
    LONGS_EQUAL(0, bench.DeviceOperationsInLastPass());
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

TEST(ScenarioBench, AMisroutedSenderDropsTheRecordAndReportsTheLoss)
{
    ScenarioBench bench(options);
    bench.SetMisrouted(true);
    bench.Log();
    bench.Service();
    LONGS_EQUAL(0, bench.Delivered().size());
    LONGS_EQUAL(SOLIDSYSLOG_CAT_SENDER_RECORDS_LOST, bench.Events()[0].Category);
}

TEST(ScenarioBench, AReportTheMisroutedSenderDroppedIsRecorded)
{
    ScenarioBench bench(options);
    bench.SetMisrouted(true);
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, nullptr, SOLIDSYSLOG_CAT_FILE_IO_FAILED, 7);
    bench.Service();
    LONGS_EQUAL(0, bench.Reports().size());
    LONGS_EQUAL(1, bench.DroppedReports().size());
    STRCMP_EQUAL("FILE_IO_FAILED", bench.DroppedReports()[0].Category.c_str());
}

TEST(ScenarioBench, ARecordTheMisroutedSenderDroppedIsRecorded)
{
    ScenarioBench bench(options);
    bench.SetMisrouted(true);
    bench.Log();
    bench.Service();
    LONGS_EQUAL(1, bench.DroppedRecords().size());
    LONGS_EQUAL(1, bench.DroppedRecords()[0].Ordinal);
    LONGS_EQUAL(1, bench.DroppedRecords()[0].Pass);
}

TEST(ScenarioBench, ASenderRoutedBackDeliversAgain)
{
    ScenarioBench bench(options);
    bench.SetMisrouted(true);
    bench.Log();
    bench.Service();
    bench.SetMisrouted(false);
    bench.Log();
    bench.Service(); /* sends the report of the loss, stored ahead of the record */
    bench.Service();
    LONGS_EQUAL(1, bench.Delivered().size());
    LONGS_EQUAL(2, bench.Delivered()[0].Ordinal);
}

TEST(ScenarioBench, ARefusedHandshakeIsRaisedAheadOfTheFailedDelivery)
{
    ScenarioBench bench(options);
    bench.Log();
    bench.Service();
    bench.SetHandshakeRefused(true);
    bench.Log();
    bench.Service();
    LONGS_EQUAL(1, bench.Delivered().size());
    LONGS_EQUAL(2, bench.Events().size());
    LONGS_EQUAL(SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED, bench.Events()[0].Category);
    LONGS_EQUAL(SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED, bench.Events()[1].Category);
}

TEST(ScenarioBench, AnAcceptedHandshakeDeliversTheRefusalReportedAheadOfTheOutage)
{
    ScenarioBench bench(options);
    bench.SetHandshakeRefused(true);
    bench.Log();
    bench.Service();
    bench.SetHandshakeRefused(false);
    for (int i = 0; i < 5; i++)
    {
        bench.Service();
    }
    LONGS_EQUAL(1, bench.Delivered().size());
    STRCMP_EQUAL("TLS_STREAM_HANDSHAKE_FAILED", bench.Reports()[0].Category.c_str());
    STRCMP_EQUAL("SENDER_DELIVERY_FAILED", bench.Reports()[1].Category.c_str());
}

TEST(ScenarioBench, APassRecordsTheOpensItTried)
{
    ScenarioBench bench(options);
    bench.SetHandshakeRefused(true);
    bench.Log();
    bench.Service();
    LONGS_EQUAL(1, bench.OpensInLastPass());
    LONGS_EQUAL(0, bench.SendsInLastPass());
}

TEST(ScenarioBench, AFileBackedStoreKeepsWhatTheSenderRefusedAndDeliversIt)
{
    options.FileBacked = true;
    ScenarioBench bench(options);
    bench.SetSenderDown(true);
    bench.Log();
    bench.Log();
    bench.Service();
    bench.SetSenderDown(false);
    for (int pass = 0; pass < 4; pass++)
    {
        bench.Service();
    }
    LONGS_EQUAL(2, bench.Delivered().size());
}

TEST(ScenarioBench, AFailedFileIsReportedWithItsErrnoBeforeTheStoreFreezes)
{
    options.FileBacked = true;
    ScenarioBench bench(options);
    bench.Log();
    bench.Service();
    bench.FailDevice();
    bench.Log();
    bench.Service();
    LONGS_EQUAL(SOLIDSYSLOG_CAT_FILE_IO_FAILED, bench.Events().front().Category);
    STRCMP_EQUAL("5", bench.Events().front().NativeParam.c_str());
    LONGS_EQUAL(SOLIDSYSLOG_CAT_STORE_FROZEN, bench.Events().back().Category);
}

TEST(ScenarioBench, AWithheldKeyLosesTheRecordTheStoreCannotSeal)
{
    ScenarioBench bench(options);
    bench.SetSecurityPolicyFault(SECURITY_POLICY_FAKE_KEY_WITHHELD);
    bench.Log();
    bench.Service();
    LONGS_EQUAL(2, bench.Events().size());
    LONGS_EQUAL(SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE, bench.Events()[0].Category);
    LONGS_EQUAL(SOLIDSYSLOG_CAT_STORE_WRITE_FAILED, bench.Events()[1].Category);
}

TEST(ScenarioBench, ARecordThatFailsVerificationIsDiscarded)
{
    ScenarioBench bench(options);
    bench.SetSecurityPolicyFault(SECURITY_POLICY_FAKE_REJECTING);
    bench.Log();
    bench.Service();
    LONGS_EQUAL(1, bench.Events().size());
    LONGS_EQUAL(SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED, bench.Events()[0].Category);
}

TEST(ScenarioBench, ACorruptRingIsReportedAsItsBackendFailing)
{
    ScenarioBench bench(options);
    bench.Log();
    bench.CorruptTheRing();
    bench.Service();
    LONGS_EQUAL(1, bench.Events().size());
    LONGS_EQUAL(SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED, bench.Events()[0].Category);
}

TEST(ScenarioBench, AFullBufferUnderNewestRefusesAndIsReportedOnceFromTheLog)
{
    options.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
    ScenarioBench bench(options);
    FillTheRing(bench);
    LONGS_EQUAL(1, bench.Events().size());
    LONGS_EQUAL(SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, bench.Events()[0].Category);
    LONGS_EQUAL(0, bench.Events()[0].Pass);
}

TEST(ScenarioBench, ALossTheLoggerReportsIsAReportFrameNotGarbage)
{
    options.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
    ScenarioBench bench(options);
    FillTheRing(bench);
    bench.Service();
    LONGS_EQUAL(0, bench.Garbage());
    LONGS_EQUAL(1, bench.Reports().size());
    STRCMP_EQUAL("BUFFER_RECORDS_LOST", bench.Reports()[0].Category.c_str());
    LONGS_EQUAL(1, bench.Reports()[0].Pass);
}

TEST(ScenarioBench, AReportCarriesTheParamsItWasWrittenWith)
{
    options.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
    ScenarioBench bench(options);
    FillTheRing(bench);
    bench.Service();
    STRCMP_EQUAL("CircularBuffer", bench.Reports()[0].SourceParam.c_str());
    STRCMP_EQUAL("RECORD_REFUSED", bench.Reports()[0].DetailParam.c_str());
    STRCMP_EQUAL("", bench.Reports()[0].NativeParam.c_str());
}

TEST(ScenarioBench, AReportCarriesItsEventsNativeCode)
{
    ScenarioBench bench(options);
    SolidSyslog_ErrorWithNativeCode(SOLIDSYSLOG_SEVERITY_ERROR, nullptr, SOLIDSYSLOG_CAT_FILE_IO_FAILED, 7, -5);
    bench.Service();
    STRCMP_EQUAL("-5", bench.Reports()[0].NativeParam.c_str());
}

TEST(ScenarioBench, AReportTakesTheSequenceIdAfterTheRecordsLogged)
{
    options.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
    ScenarioBench bench(options);
    FillTheRing(bench);
    bench.Service();
    LONGS_EQUAL(bench.Logged() + 1U, bench.Reports()[0].SequenceId);
}

TEST(ScenarioBench, WithReportingOffTheLoggerMakesNoReport)
{
    options.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
    ScenarioBench bench(options);
    bench.SetReporting(false);
    FillTheRing(bench);
    bench.Service();
    LONGS_EQUAL(0, bench.Reports().size());
}

TEST(ScenarioBench, AReportMadeIsCounted)
{
    options.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
    ScenarioBench bench(options);
    FillTheRing(bench);
    bench.Service();
    LONGS_EQUAL(1, bench.ReportsMade());
}

TEST(ScenarioBench, AReportTurnedDownIsNotCountedAsMade)
{
    options.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
    ScenarioBench bench(options);
    bench.SetReporting(false);
    FillTheRing(bench);
    bench.Service();
    LONGS_EQUAL(0, bench.ReportsMade());
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
