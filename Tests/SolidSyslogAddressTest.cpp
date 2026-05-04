#include "SolidSyslogAddress.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(SolidSyslogAddress)
{
    SolidSyslogAddressStorage storage{};
};

// clang-format on

TEST(SolidSyslogAddress, FromStorageReturnsNonNull)
{
    CHECK_TRUE(SolidSyslogAddress_FromStorage(&storage) != nullptr);
}

TEST(SolidSyslogAddress, FromStorageReturnsSamePointerOnSuccessiveCalls)
{
    struct SolidSyslogAddress* first  = SolidSyslogAddress_FromStorage(&storage);
    struct SolidSyslogAddress* second = SolidSyslogAddress_FromStorage(&storage);
    POINTERS_EQUAL(first, second);
}
