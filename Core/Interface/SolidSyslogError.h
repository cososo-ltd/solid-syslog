#ifndef SOLIDSYSLOGERROR_H
#define SOLIDSYSLOGERROR_H

#include <stdint.h>

#include "ExternC.h"
#include "SolidSyslogPrival.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource
    {
        const char* Name;
        const char* (*AsString)(uint8_t code);
    };

    struct SolidSyslogErrorEvent
    {
        enum SolidSyslogSeverity Severity;
        const struct SolidSyslogErrorSource* Source;
        uint16_t Category;
        int32_t Detail;
    };

    typedef void (*SolidSyslogErrorHandler)(void* context, const struct SolidSyslogErrorEvent* event);

    void SolidSyslog_SetErrorHandler(SolidSyslogErrorHandler handler, void* context);
    void SolidSyslog_Error(
        enum SolidSyslogSeverity severity,
        const struct SolidSyslogErrorSource* source,
        uint16_t category,
        int32_t detail
    );

EXTERN_C_END

#endif /* SOLIDSYSLOGERROR_H */
