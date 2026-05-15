#ifndef SOLIDSYSLOGSECURITYPOLICYDEFINITION_H
#define SOLIDSYSLOGSECURITYPOLICYDEFINITION_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

EXTERN_C_BEGIN

    /* large enough for HMAC-SHA256 (32 bytes); CRC-16 uses 2 */
    enum
    {
        SOLIDSYSLOG_MAX_INTEGRITY_SIZE = 32
    };

    struct SolidSyslogSecurityPolicy
    {
        uint16_t IntegritySize;
        void (*ComputeIntegrity)(const uint8_t* data, uint16_t length, uint8_t* integrityOut);
        bool (*VerifyIntegrity)(const uint8_t* data, uint16_t length, const uint8_t* integrityIn);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGSECURITYPOLICYDEFINITION_H */
