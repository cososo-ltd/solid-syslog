#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "lwip/sockets.h"

#include "SolidSyslogLwipSocketAddress.h"

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketAddress)
{
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        address = SolidSyslogLwipSocketAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogLwipSocketAddress_Destroy(address);
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketAddress, CreateReturnsNonNull)
{
    CHECK(address != nullptr);
}
