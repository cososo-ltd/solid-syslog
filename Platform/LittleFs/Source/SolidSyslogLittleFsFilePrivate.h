/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGLITTLEFSFILEPRIVATE_H
#define SOLIDSYSLOGLITTLEFSFILEPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogLittleFsFileErrors.h"
#include "SolidSyslogPrival.h"
#include "lfs.h"

struct SolidSyslogLittleFsFile
{
    struct SolidSyslogFile Base;
    lfs_t* Filesystem;
    lfs_file_t Handle;
    struct lfs_file_config OpenConfig;
    bool IsOpen;
};

bool SolidSyslogLittleFsFile_Initialise(
    struct SolidSyslogFile* base,
    lfs_t* filesystem,
    void* fileBuffer,
    uint32_t fileBufferBytes
);
void SolidSyslogLittleFsFile_Cleanup(struct SolidSyslogFile* base);

static inline void LittleFsFile_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogFileErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogLittleFsFileErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGLITTLEFSFILEPRIVATE_H */
