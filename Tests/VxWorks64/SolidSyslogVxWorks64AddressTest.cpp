#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "SolidSyslogVxWorks64Address.h"

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64Address)
{
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        address = SolidSyslogVxWorks64Address_Create();
    }

    void teardown() override
    {
        SolidSyslogVxWorks64Address_Destroy(address);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Address, CreateReturnsNonNull)
{
    CHECK(address != nullptr);
}
