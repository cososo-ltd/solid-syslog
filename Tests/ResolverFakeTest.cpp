#include "AddressFake.h"
#include "ResolverFake.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogTransport.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(ResolverFake){};

TEST(ResolverFake, ResolvesWhateverItIsAsked)
{
    CHECK_TRUE(SolidSyslogResolver_Resolve(ResolverFake_Get(), SOLIDSYSLOG_TRANSPORT_TCP, "any", 514, AddressFake_Get())
    );
}

TEST(ResolverFake, CountsResolves)
{
    SolidSyslogResolver_Resolve(ResolverFake_Get(), SOLIDSYSLOG_TRANSPORT_TCP, "any", 514, AddressFake_Get());
    LONGS_EQUAL(1, ResolverFake_ResolveCallCount());
}
