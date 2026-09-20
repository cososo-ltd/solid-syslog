/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogLittleFsFile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogLittleFsFilePrivate.h"
#include "SolidSyslogNullFile.h"
#include "lfs.h"

const struct SolidSyslogErrorSource SolidSyslogLittleFsFileErrorSource = {"LittleFsFile"};

static bool LittleFsFile_IsOpen(struct SolidSyslogFile* base);

static inline struct SolidSyslogLittleFsFile* LittleFsFile_SelfFromBase(struct SolidSyslogFile* base);

bool SolidSyslogLittleFsFile_Initialise(
    struct SolidSyslogFile* base,
    lfs_t* filesystem,
    void* fileBuffer,
    uint32_t fileBufferBytes
)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    (void) fileBufferBytes;
    /* Start from the Null vtable so any slot this adapter has not filled is a
     * safe no-op rather than a NULL dispatch. */
    self->Base = *SolidSyslogNullFile_Get();
    self->Filesystem = filesystem;
    self->OpenConfig.buffer = fileBuffer;
    self->Base.IsOpen = LittleFsFile_IsOpen;
    self->IsOpen = false;
    return true;
}

static inline struct SolidSyslogLittleFsFile* LittleFsFile_SelfFromBase(struct SolidSyslogFile* base)
{
    return (struct SolidSyslogLittleFsFile*) base;
}

void SolidSyslogLittleFsFile_Cleanup(struct SolidSyslogFile* base)
{
    /* Overwrite the abstract base with the shared NullFile vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullFile_Get();
}

static bool LittleFsFile_IsOpen(struct SolidSyslogFile* base)
{
    return LittleFsFile_SelfFromBase(base)->IsOpen;
}
