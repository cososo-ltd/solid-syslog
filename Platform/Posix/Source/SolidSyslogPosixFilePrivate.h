#ifndef SOLIDSYSLOGPOSIXFILEPRIVATE_H
#define SOLIDSYSLOGPOSIXFILEPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogPosixFileErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogPosixFile
{
    struct SolidSyslogFile Base;
    int Fd;
};

void PosixFile_Initialise(struct SolidSyslogFile* base);
void PosixFile_Cleanup(struct SolidSyslogFile* base);

static inline void PosixFile_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPosixFileErrors code
)
{
    SolidSyslog_Error(severity, &PosixFileErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPOSIXFILEPRIVATE_H */
