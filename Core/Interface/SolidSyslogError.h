#ifndef SOLIDSYSLOGERROR_H
#define SOLIDSYSLOGERROR_H

#include "ExternC.h"
#include "SolidSyslogPrival.h"

EXTERN_C_BEGIN

    typedef void (*SolidSyslogErrorHandler)(void* context, enum SolidSyslogSeverity severity, const char* message);

    void SolidSyslog_SetErrorHandler(SolidSyslogErrorHandler handler, void* context);
    void SolidSyslog_Error(enum SolidSyslogSeverity severity, const char* message);

EXTERN_C_END

#endif /* SOLIDSYSLOGERROR_H */
