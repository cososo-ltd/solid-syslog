#ifndef SOLIDSYSLOGWINDOWSATOMICCOUNTERPRIVATE_H
#define SOLIDSYSLOGWINDOWSATOMICCOUNTERPRIVATE_H

#include <stdint.h>
#include <windows.h>

#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogWindowsAtomicCounterErrors.h"

struct SolidSyslogWindowsAtomicCounter
{
    struct SolidSyslogAtomicCounter Base;
    volatile LONG Value;
};

void WindowsAtomicCounter_Initialise(struct SolidSyslogAtomicCounter* base);
void WindowsAtomicCounter_Cleanup(struct SolidSyslogAtomicCounter* base);

static inline void WindowsAtomicCounter_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogWindowsAtomicCounterErrors code
)
{
    SolidSyslog_Error(severity, &WindowsAtomicCounterErrorSource, category, code);
}

#endif /* SOLIDSYSLOGWINDOWSATOMICCOUNTERPRIVATE_H */
