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
    const struct SolidSyslogSecurityRecord* record
);
static bool Crc16Policy_Crc16OpenRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
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

/* CRC-16 is a checksum, not an AEAD — it covers the whole content and has no
 * use for the header/body split, so HeaderLength is ignored. It detects
 * accidental corruption; it authenticates nothing. */
static bool Crc16Policy_Crc16SealRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
)
{
    (void) self;
    uint16_t crc = SolidSyslogCrc16_Compute(record->Content, record->ContentLength);
    record->Trailer[0] = (uint8_t) (crc >> 8U);
    record->Trailer[1] = (uint8_t) (crc & 0xFFU);
    return true;
}

static bool Crc16Policy_Crc16OpenRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
)
{
    (void) self;
    uint16_t crc = SolidSyslogCrc16_Compute(record->Content, record->ContentLength);
    uint16_t expected = (uint16_t) ((uint16_t) (record->Trailer[0] << 8U) | record->Trailer[1]);
    return crc == expected;
}
