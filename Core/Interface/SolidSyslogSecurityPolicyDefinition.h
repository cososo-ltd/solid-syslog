#ifndef SOLIDSYSLOGSECURITYPOLICYDEFINITION_H
#define SOLIDSYSLOGSECURITYPOLICYDEFINITION_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

EXTERN_C_BEGIN

    /* A record is laid out as one contiguous span the policy sees as
     *   content[0 .. headerLength)             associated data — authenticated,
     *                                          never encrypted (the cleartext
     *                                          header the reader needs intact)
     *   content[headerLength .. contentLength) body — authenticated; an AEAD
     *                                          policy also encrypts it in place
     * plus a separate trailer of TrailerSize bytes the policy owns. MAC and
     * checksum policies authenticate the whole content and ignore the split;
     * AEAD policies treat the header as associated data and encrypt the body. */
    struct SolidSyslogSecurityPolicy
    {
        uint16_t TrailerSize;
        bool (*SealRecord)(
            struct SolidSyslogSecurityPolicy* self,
            uint8_t* content,
            uint16_t contentLength,
            uint16_t headerLength,
            uint8_t* trailerOut
        );
        bool (*OpenRecord)(
            struct SolidSyslogSecurityPolicy* self,
            uint8_t* content,
            uint16_t contentLength,
            uint16_t headerLength,
            const uint8_t* trailerIn
        );
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGSECURITYPOLICYDEFINITION_H */
