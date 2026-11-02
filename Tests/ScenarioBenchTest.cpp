#include "ScenarioBench.h"
#include "SolidSyslogDiscardPolicy.h"
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
