#ifndef SOLIDSYSLOGDATAGRAMDEFINITION_H
#define SOLIDSYSLOGDATAGRAMDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogDatagram.h"
#include "ExternC.h"

struct SolidSyslogAddress;

EXTERN_C_BEGIN

    struct SolidSyslogDatagram
    {
        bool (*Open)(struct SolidSyslogDatagram* self);
        enum SolidSyslogDatagramSendResult (*SendTo)(struct SolidSyslogDatagram* self, const void* buffer, size_t size, const struct SolidSyslogAddress* addr);
        size_t (*MaxPayload)(struct SolidSyslogDatagram* self);
        void (*Close)(struct SolidSyslogDatagram* self);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGDATAGRAMDEFINITION_H */
