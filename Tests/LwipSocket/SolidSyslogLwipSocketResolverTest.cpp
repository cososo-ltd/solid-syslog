#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "lwip/sockets.h"

#include "SolidSyslogLwipSocketResolver.h"

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketResolver)
{
    struct SolidSyslogResolver* resolver = nullptr;

    void setup() override
    {
        resolver = SolidSyslogLwipSocketResolver_Create();
    }

    void teardown() override
    {
        SolidSyslogLwipSocketResolver_Destroy(resolver);
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketResolver, CreateReturnsNonNull)
{
    CHECK(resolver != nullptr);
}
