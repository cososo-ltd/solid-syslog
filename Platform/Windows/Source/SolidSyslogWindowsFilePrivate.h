#ifndef SOLIDSYSLOGWINDOWSFILEPRIVATE_H
#define SOLIDSYSLOGWINDOWSFILEPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogWindowsFileErrors.h"

struct SolidSyslogWindowsFile
{
    struct SolidSyslogFile Base;
    int Fd;
};

void WindowsFile_Initialise(struct SolidSyslogFile* base);
void WindowsFile_Cleanup(struct SolidSyslogFile* base);

static inline void WindowsFile_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogWindowsFileErrors code
)
{
    SolidSyslog_Error(severity, &WindowsFileErrorSource, category, code);
}

#endif /* SOLIDSYSLOGWINDOWSFILEPRIVATE_H */
