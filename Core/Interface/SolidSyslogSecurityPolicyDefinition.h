#ifndef SOLIDSYSLOGSECURITYPOLICYDEFINITION_H
#define SOLIDSYSLOGSECURITYPOLICYDEFINITION_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

EXTERN_C_BEGIN

    struct SolidSyslogSecurityPolicy
    {
        uint16_t IntegritySize;
        bool (*ComputeIntegrity)(
            struct SolidSyslogSecurityPolicy* self,
            const uint8_t* data,
            uint16_t length,
            uint8_t* integrityOut
        );
        bool (*VerifyIntegrity)(
            struct SolidSyslogSecurityPolicy* self,
            const uint8_t* data,
            uint16_t length,
            const uint8_t* integrityIn
        );
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGSECURITYPOLICYDEFINITION_H */
