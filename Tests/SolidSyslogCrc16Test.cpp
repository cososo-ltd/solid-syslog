#include <stdint.h>
#include <cstring>

#include "SolidSyslogCrc16.h"
#include "SolidSyslogTunables.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(SolidSyslogCrc16)
{
};

// clang-format on

/* Check value 0x29B1 from Greg Cook's CRC catalogue:
 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-ibm-3740 */
TEST(SolidSyslogCrc16, KnownTestVector)
{
    const uint8_t data[] = "123456789";
    LONGS_EQUAL(0x29B1, SolidSyslogCrc16_Compute(data, 9));
}

/* Regression: init value 0xFFFF returned unchanged for zero-length input */
TEST(SolidSyslogCrc16, EmptyInputReturnsInitValue)
{
    LONGS_EQUAL(0xFFFF, SolidSyslogCrc16_Compute(nullptr, 0));
}

/* Regression: single-byte boundary */
TEST(SolidSyslogCrc16, SingleByteZero)
{
    const uint8_t data[] = {0x00};
    LONGS_EQUAL(0xE1F0, SolidSyslogCrc16_Compute(data, 1));
}

TEST(SolidSyslogCrc16, FlippingAnyByteChangesCrc)
{
    const uint8_t original[] = {0x01, 0x02, 0x03, 0x04, 0x05};

    enum
    {
        DATA_LEN = 5
    };

    uint16_t originalCrc = SolidSyslogCrc16_Compute(original, DATA_LEN);

    for (int i = 0; i < DATA_LEN; i++)
    {
        uint8_t modified[DATA_LEN];
        memcpy(modified, original, DATA_LEN);
        modified[i] ^= 0x01; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        CHECK_TRUE(SolidSyslogCrc16_Compute(modified, DATA_LEN) != originalCrc);
    }
}

TEST(SolidSyslogCrc16, BeyondMaxMessageSize)
{
    uint8_t data[SOLIDSYSLOG_MAX_MESSAGE_SIZE + 1];
    memset(data, 'A', sizeof(data));
    uint16_t crc = SolidSyslogCrc16_Compute(data, sizeof(data));

    data[SOLIDSYSLOG_MAX_MESSAGE_SIZE] ^= 0x01;
    CHECK_TRUE(SolidSyslogCrc16_Compute(data, sizeof(data)) != crc);
}
