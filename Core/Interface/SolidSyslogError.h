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

    typedef void (*SolidSyslogErrorHandler)(void* context, enum SolidSyslogSeverity severity, const char* message);
    typedef void (*SolidSyslogErrorHandlerEx)(
        void* context,
        enum SolidSyslogSeverity severity,
        const struct SolidSyslogErrorSource* source,
        uint8_t code
    );

    void SolidSyslog_SetErrorHandler(SolidSyslogErrorHandler handler, void* context);
    void SolidSyslog_SetErrorHandlerEx(SolidSyslogErrorHandlerEx handler, void* context);
    void SolidSyslog_Error(enum SolidSyslogSeverity severity, const char* message);
    void SolidSyslog_ErrorEx(
        enum SolidSyslogSeverity severity,
        const struct SolidSyslogErrorSource* source,
        uint8_t code
    );

EXTERN_C_END

#endif /* SOLIDSYSLOGERROR_H */
