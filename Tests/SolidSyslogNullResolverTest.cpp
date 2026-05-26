#include "CppUTest/TestHarness.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogTransport.h"

// clang-format off
TEST_GROUP(SolidSyslogNullResolver)
{
    struct SolidSyslogResolver* resolver = nullptr;

    void setup() override
    {
        resolver = SolidSyslogNullResolver_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullResolver, ResolveReturnsFalse)
{
    CHECK_FALSE(SolidSyslogResolver_Resolve(resolver, SOLIDSYSLOG_TRANSPORT_UDP, "anywhere", 514U, nullptr));
}
