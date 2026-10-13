#include "BddTargetKey.h"
#include "CppUTest/TestHarness.h"

#include <stddef.h>
#include <stdint.h>

// clang-format off
TEST_GROUP(BddTargetKey)
{
    uint8_t key[64] = {};
    size_t keyLength = 0U;

    void setup() override
    {
        BddTargetKey_Reset();
    }
};

// clang-format on

TEST(BddTargetKey, GetGivesThirtyTwoBytesOfTheDemoKey)
{
    CHECK_TRUE(BddTargetKey_Get(nullptr, key, sizeof key, &keyLength));

    LONGS_EQUAL(32, keyLength);
    BYTES_EQUAL(0x5A, key[0]);
    BYTES_EQUAL(0x5A, key[31]);
}

TEST(BddTargetKey, GetWritesNoMoreThanItIsGivenRoomFor)
{
    key[16] = 0x00;

    BddTargetKey_Get(nullptr, key, 16, &keyLength);

    LONGS_EQUAL(16, keyLength);
    BYTES_EQUAL(0x00, key[16]);
}

TEST(BddTargetKey, AWithheldKeyIsUnavailable)
{
    CHECK_TRUE(BddTargetKey_SetByName("key", "withheld"));

    CHECK_FALSE(BddTargetKey_Get(nullptr, key, sizeof key, &keyLength));
}

TEST(BddTargetKey, AKeyMadeAvailableAgainIsGiven)
{
    BddTargetKey_SetByName("key", "withheld");

    CHECK_TRUE(BddTargetKey_SetByName("key", "available"));

    CHECK_TRUE(BddTargetKey_Get(nullptr, key, sizeof key, &keyLength));
}

TEST(BddTargetKey, AValueItDoesNotKnowIsRefused)
{
    CHECK_FALSE(BddTargetKey_SetByName("key", "lost"));

    CHECK_TRUE(BddTargetKey_Get(nullptr, key, sizeof key, &keyLength));
}

TEST(BddTargetKey, ANameItDoesNotHoldIsNotTaken)
{
    CHECK_FALSE(BddTargetKey_SetByName("msgid", "withheld"));

    CHECK_TRUE(BddTargetKey_Get(nullptr, key, sizeof key, &keyLength));
}
