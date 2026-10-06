/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGVXWORKS64FILEPRIVATE_H
#define SOLIDSYSLOGVXWORKS64FILEPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogVxWorks64FileErrors.h"

struct SolidSyslogVxWorks64File
{
    struct SolidSyslogFile Base;
    int Fd;
};

void SolidSyslogVxWorks64File_Initialise(struct SolidSyslogFile* base);
void SolidSyslogVxWorks64File_Cleanup(struct SolidSyslogFile* base);

static inline void VxWorks64File_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogFileErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogVxWorks64FileErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGVXWORKS64FILEPRIVATE_H */
