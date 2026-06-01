#include "SolidSyslogCrc16Policy.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogCrc16.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

enum
{
    CRC16_SIZE = 2
};

static bool Crc16Policy_Crc16SealRecord(
    struct SolidSyslogSecurityPolicy* self,
    uint8_t* content,
    uint16_t contentLength,
    uint16_t headerLength,
    uint8_t* trailerOut
);
static bool Crc16Policy_Crc16OpenRecord(
    struct SolidSyslogSecurityPolicy* self,
    uint8_t* content,
    uint16_t contentLength,
    uint16_t headerLength,
    const uint8_t* trailerIn
);

struct SolidSyslogSecurityPolicy* SolidSyslogCrc16Policy_Create(void)
{
    static struct SolidSyslogSecurityPolicy instance = {
        CRC16_SIZE,
        Crc16Policy_Crc16SealRecord,
        Crc16Policy_Crc16OpenRecord,
    };
    return &instance;
}

void SolidSyslogCrc16Policy_Destroy(void)
{
}

/* CRC-16 is a checksum, not an AEAD — it authenticates the whole content and
 * has no use for the header/body split, so headerLength is ignored. */
static bool Crc16Policy_Crc16SealRecord(
    struct SolidSyslogSecurityPolicy* self,
    // NOLINTNEXTLINE(readability-non-const-parameter) -- content is non-const to match the vtable signature
    uint8_t* content,
    uint16_t contentLength,
    uint16_t headerLength,
    uint8_t* trailerOut
)
{
    (void) self;
    (void) headerLength;
    uint16_t crc = SolidSyslogCrc16_Compute(content, contentLength);
    trailerOut[0] = (uint8_t) (crc >> 8U);
    trailerOut[1] = (uint8_t) (crc & 0xFFU);
    return true;
}

static bool Crc16Policy_Crc16OpenRecord(
    struct SolidSyslogSecurityPolicy* self,
    // NOLINTNEXTLINE(readability-non-const-parameter) -- content is non-const to match the vtable signature
    uint8_t* content,
    uint16_t contentLength,
    uint16_t headerLength,
    const uint8_t* trailerIn
)
{
    (void) self;
    (void) headerLength;
    uint16_t crc = SolidSyslogCrc16_Compute(content, contentLength);
    uint16_t expected = (uint16_t) ((uint16_t) (trailerIn[0] << 8U) | trailerIn[1]);
    return crc == expected;
}
