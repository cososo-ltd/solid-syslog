#ifndef SOLIDSYSLOGWINDOWSMUTEXPRIVATE_H
#define SOLIDSYSLOGWINDOWSMUTEXPRIVATE_H

#include <stdint.h>

#include <windows.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogWindowsMutexErrors.h"

struct SolidSyslogWindowsMutex
{
    struct SolidSyslogMutex Base;
    CRITICAL_SECTION Section;
};

void WindowsMutex_Initialise(struct SolidSyslogMutex* base);
void WindowsMutex_Cleanup(struct SolidSyslogMutex* base);

static inline void WindowsMutex_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogWindowsMutexErrors code
)
{
    SolidSyslog_Error(severity, &WindowsMutexErrorSource, category, code);
}

#endif /* SOLIDSYSLOGWINDOWSMUTEXPRIVATE_H */
