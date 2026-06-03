#ifndef SOLIDSYSLOGFATFSFILEPRIVATE_H
#define SOLIDSYSLOGFATFSFILEPRIVATE_H

#include <stdint.h>

#include <stdbool.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFatFsFileErrors.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogPrival.h"
#include "ff.h"

struct SolidSyslogFatFsFile
{
    struct SolidSyslogFile Base;
    FIL Fp;
    bool IsOpen;
};

void FatFsFile_Initialise(struct SolidSyslogFile* base);
void FatFsFile_Cleanup(struct SolidSyslogFile* base);

static inline void FatFsFile_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogFatFsFileErrors code
)
{
    SolidSyslog_Error(severity, &FatFsFileErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGFATFSFILEPRIVATE_H */
