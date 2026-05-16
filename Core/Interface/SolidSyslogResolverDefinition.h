#ifndef SOLIDSYSLOGRESOLVERDEFINITION_H
#define SOLIDSYSLOGRESOLVERDEFINITION_H

#include <stdint.h>
#include <stdbool.h>

#include "SolidSyslogTransport.h"
#include "ExternC.h"

struct SolidSyslogAddress;

EXTERN_C_BEGIN

    struct SolidSyslogResolver
    {
        bool (*Resolve)(
            struct SolidSyslogResolver* base,
            enum SolidSyslogTransport transport,
            const char* host,
            uint16_t port,
            struct SolidSyslogAddress* result
        );
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGRESOLVERDEFINITION_H */
