#include "StoreBench.h"
#include "SolidSyslogBlockDevice.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(StoreBench)
{
    StoreBenchOptions options = {1024, 2, nullptr, 32};
};

// clang-format on

TEST(StoreBench, AFreshBenchHasDeliveredNothing)
{
    StoreBench bench(options);
    CHECK_TRUE(bench.Delivered().empty());
}

TEST(StoreBench, AKeptRecordDrainedIsDelivered)
{
    StoreBench bench(options);
    LONGS_EQUAL(SOLIDSYSLOG_STORE_WRITE_KEPT, bench.Write(7));
    bench.Drain(10);
    LONGS_EQUAL(1, bench.Delivered().size());
    LONGS_EQUAL(7, bench.Delivered()[0]);
}

TEST(StoreBench, ARecordDrainedAfterARestartIsDelivered)
{
    StoreBench bench(options);
    bench.Write(7);
    bench.Restart();
    bench.Drain(10);
    LONGS_EQUAL(1, bench.Delivered().size());
    LONGS_EQUAL(7, bench.Delivered()[0]);
}

TEST(StoreBench, AKeptRecordNotDeliveredIsSilentlyLostUntilItIs)
{
    StoreBench bench(options);
    bench.Write(7);
    LONGS_EQUAL(1, bench.SilentlyLost().size());
    LONGS_EQUAL(7, bench.SilentlyLost()[0]);
    bench.Drain(10);
    CHECK_TRUE(bench.SilentlyLost().empty());
}

TEST(StoreBench, ALossTheStoreReportsIsNotSilent)
{
    StoreBench bench(options);
    bench.Write(7);
    BlockDeviceFake_FlipBit(0, 0, 0); /* the record's first magic byte */
    bench.Drain(10);
    CHECK_TRUE(bench.Delivered().empty());
    LONGS_EQUAL(1, bench.Lost().size());
    LONGS_EQUAL(7, bench.Lost()[0]);
    LONGS_EQUAL(1, bench.LossReports());
    CHECK_TRUE(bench.SilentlyLost().empty());
}

TEST(StoreBench, GarbageDeliveredUnderTheNullPolicyIsCountedAndNotSilent)
{
    StoreBench bench(options);
    bench.Write(7);
    BlockDeviceFake_FlipBit(0, 8, 6); /* the first sequence digit, past magic, length and "SEQ=" */
    bench.Drain(10);
    CHECK_TRUE(bench.Delivered().empty());
    LONGS_EQUAL(1, bench.Garbage());
    LONGS_EQUAL(0, bench.LossReports());
    CHECK_TRUE(bench.SilentlyLost().empty());
}

TEST(StoreBench, ARecordReadButNotMarkedBeforeARestartIsDeliveredTwiceAndCountedAsADuplicate)
{
    StoreBench bench(options);
    bench.Write(7);
    CHECK_TRUE(bench.ReadWithoutMarking());
    bench.Restart();
    bench.Drain(10);
    LONGS_EQUAL(2, bench.Delivered().size());
    LONGS_EQUAL(1, bench.Duplicates());
}

TEST(StoreBench, TheLargestOperationCountOfOneStoreCallIsKept)
{
    StoreBench bench(options);
    LONGS_EQUAL(101, bench.MaxOperationsInOneCall()); /* Create scans 100 blocks and acquires one */
    bench.Write(7);
    bench.Drain(10);
    LONGS_EQUAL(101, bench.MaxOperationsInOneCall());
}

TEST(StoreBench, AWriteAtThatIsNotAFlagByteAtARecordEndIsIllegal)
{
    StoreBench bench(options);
    bench.Write(7);
    bench.Drain(10);
    CHECK_TRUE(bench.IllegalWrites().empty());
    SolidSyslogBlockDevice_WriteAt(bench.Device(), 0, 1, "Z", 1);
    LONGS_EQUAL(1, bench.IllegalWrites().size());
    LONGS_EQUAL(1, bench.IllegalWrites()[0].Offset);
}

TEST(StoreBench, ALostRecordOutsideTheFaultedBlockIsNamed)
{
    /* One worst-case record per block, so the second write rotates to block 1. */
    StoreBenchOptions oneRecordPerBlock = {485, 3, nullptr, 480};
    StoreBench bench(oneRecordPerBlock);
    bench.Write(1);
    bench.Write(2);
    BlockDeviceFake_FlipBit(1, 0, 0);
    bench.Drain(10);
    LONGS_EQUAL(1, bench.Lost().size());
    CHECK_TRUE(bench.LostOutsideBlock(1).empty());
    LONGS_EQUAL(1, bench.LostOutsideBlock(0).size());
    LONGS_EQUAL(2, bench.LostOutsideBlock(0)[0]);
}

TEST(StoreBench, ALostRecordStillReadableOnTheMediaIsNotGone)
{
    StoreBenchOptions oneRecordPerBlock = {485, 3, nullptr, 480};
    StoreBench bench(oneRecordPerBlock);
    bench.Write(1);
    bench.Write(2);
    BlockDeviceFake_FailReadsTouching(0, 0); /* the store freezes on the older block */
    bench.Drain(10);
    LONGS_EQUAL(1, bench.FreezeReports());
    LONGS_EQUAL(1, bench.LostOutsideBlock(0).size());
    CHECK_TRUE(bench.GoneOutsideBlock(0).empty());
}

TEST(StoreBench, ALostRecordWhoseBlockWasRemovedIsGone)
{
    StoreBenchOptions oneRecordPerBlock = {485, 3, nullptr, 480};
    StoreBench bench(oneRecordPerBlock);
    bench.Write(1);
    bench.Write(2);
    BlockDeviceFake_RemoveBlock(1);
    bench.Drain(10);
    LONGS_EQUAL(1, bench.GoneOutsideBlock(0).size());
    LONGS_EQUAL(2, bench.GoneOutsideBlock(0)[0]);
}

TEST(StoreBench, BytesWrittenAfterTheFaultCountsOnlyWhatFollowedTheMark)
{
    StoreBench bench(options);
    bench.Write(7);
    bench.MarkFaultApplied();
    LONGS_EQUAL(0, bench.BytesWrittenAfterFault());
    bench.Drain(10); /* one sent-flag byte */
    LONGS_EQUAL(1, bench.BytesWrittenAfterFault());
}

TEST(StoreBench, DeliverSendsOnlyTheGivenNumberOfRecords)
{
    StoreBench bench(options);
    bench.Write(1);
    bench.Write(2);
    bench.Write(3);
    LONGS_EQUAL(2, bench.Deliver(2));
    LONGS_EQUAL(2, bench.Delivered().size());
    LONGS_EQUAL(1, bench.Lost().size());
    LONGS_EQUAL(3, bench.Lost()[0]);
}

TEST(StoreBench, ADeliveredRecordWhoseSequenceWasNeverKeptIsGarbage)
{
    StoreBench bench(options);
    bench.Write(7);
    BlockDeviceFake_FlipBit(0, 11, 2); /* a sequence digit becomes another digit: 00007 reads 00047 */
    bench.Drain(10);
    CHECK_TRUE(bench.Delivered().empty());
    LONGS_EQUAL(1, bench.Garbage());
    CHECK_TRUE(bench.SilentlyLost().empty());
}

TEST(StoreBench, ARecordWhoseSequenceDigitReadsAsAnotherKeptSequenceIsGarbage)
{
    StoreBench bench(options);
    bench.Write(1);
    bench.Write(2);
    BlockDeviceFake_FlipBit(0, 12, 1); /* record 1's last digit: 00001 reads 00003, then 00002 would with bit 0 */
    BlockDeviceFake_FlipBit(0, 12, 0);
    bench.Drain(10);
    LONGS_EQUAL(1, bench.Delivered().size());
    LONGS_EQUAL(2, bench.Delivered()[0]);
    LONGS_EQUAL(1, bench.Garbage());
}

TEST(StoreBench, ARecordWithADamagedPaddingByteIsGarbage)
{
    StoreBench bench(options);
    bench.Write(7);
    BlockDeviceFake_FlipBit(0, 20, 0); /* well inside the padding */
    bench.Drain(10);
    CHECK_TRUE(bench.Delivered().empty());
    LONGS_EQUAL(1, bench.Garbage());
}

TEST(StoreBench, TheBenchKnowsWhereTheRecordsItWroteBeginAndEnd)
{
    StoreBench bench(options);
    bench.Write(1); /* 32 bytes plus 5 of framing under the Null policy */
    bench.Write(2);
    CHECK_TRUE(bench.IsRecordBoundary(0, 0));
    CHECK_TRUE(bench.IsRecordBoundary(0, 37));
    CHECK_FALSE(bench.IsRecordBoundary(0, 36));
    CHECK_TRUE(bench.IsRecordFlagByte(0, 36));
    CHECK_TRUE(bench.IsRecordFlagByte(0, 73));
    CHECK_FALSE(bench.IsRecordFlagByte(0, 37));
}

TEST(StoreBench, TheBenchKnowsWhereEachRecordCarriesItsLength)
{
    StoreBench bench(options);
    bench.Write(1); /* two magic bytes, then the two-byte length */
    bench.Write(2);
    CHECK_FALSE(bench.IsRecordLengthByte(0, 1));
    CHECK_TRUE(bench.IsRecordLengthByte(0, 2));
    CHECK_TRUE(bench.IsRecordLengthByte(0, 3));
    CHECK_FALSE(bench.IsRecordLengthByte(0, 4));
    CHECK_TRUE(bench.IsRecordLengthByte(0, 39));
    CHECK_FALSE(bench.IsRecordLengthByte(1, 2));
}
