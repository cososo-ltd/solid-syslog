#include "SolidSyslogCrc16Policy.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogCrc16.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

enum
{
    CRC16_SIZE = 2
};

static void Crc16Policy_Crc16ComputeIntegrity(const uint8_t* data, uint16_t length, uint8_t* integrityOut);
static bool Crc16Policy_Crc16VerifyIntegrity(const uint8_t* data, uint16_t length, const uint8_t* integrityIn);

struct SolidSyslogSecurityPolicy* SolidSyslogCrc16Policy_Create(void)
{
    static struct SolidSyslogSecurityPolicy instance = {
        CRC16_SIZE,
        Crc16Policy_Crc16ComputeIntegrity,
        Crc16Policy_Crc16VerifyIntegrity,
    };
    return &instance;
}

void SolidSyslogCrc16Policy_Destroy(void)
{
}

static void Crc16Policy_Crc16ComputeIntegrity(const uint8_t* data, uint16_t length, uint8_t* integrityOut)
{
    uint16_t crc = SolidSyslogCrc16_Compute(data, length);
    integrityOut[0] = (uint8_t) (crc >> 8U);
    integrityOut[1] = (uint8_t) (crc & 0xFFU);
}

static bool Crc16Policy_Crc16VerifyIntegrity(const uint8_t* data, uint16_t length, const uint8_t* integrityIn)
{
    uint16_t crc = SolidSyslogCrc16_Compute(data, length);
    uint16_t expected = (uint16_t) ((uint16_t) (integrityIn[0] << 8U) | integrityIn[1]);
    return crc == expected;
}
