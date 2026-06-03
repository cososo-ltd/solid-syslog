#include "SolidSyslogPlusFatFile.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogPlusFatFileErrors.h"
#include "SolidSyslogPlusFatFilePrivate.h"
#include "ff_stdio.h"

const struct SolidSyslogErrorSource PlusFatFileErrorSource = {"PlusFatFile"};

static bool PlusFatFile_Open(struct SolidSyslogFile* base, const char* path);
static void PlusFatFile_Close(struct SolidSyslogFile* base);
static bool PlusFatFile_IsOpen(struct SolidSyslogFile* base);
static bool PlusFatFile_Read(struct SolidSyslogFile* base, void* buf, size_t count);

static inline struct SolidSyslogPlusFatFile* PlusFatFile_SelfFromBase(struct SolidSyslogFile* base);

void PlusFatFile_Initialise(struct SolidSyslogFile* base)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    self->Base.Open = PlusFatFile_Open;
    self->Base.Close = PlusFatFile_Close;
    self->Base.IsOpen = PlusFatFile_IsOpen;
    self->Base.Read = PlusFatFile_Read;
    self->Fp = NULL;
}

static inline struct SolidSyslogPlusFatFile* PlusFatFile_SelfFromBase(struct SolidSyslogFile* base)
{
    return (struct SolidSyslogPlusFatFile*) base;
}

void PlusFatFile_Cleanup(struct SolidSyslogFile* base)
{
    PlusFatFile_Close(base);
    /* Overwrite the abstract base with the shared NullFile vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullFile_Get();
}

static bool PlusFatFile_Open(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    /* "r+" opens an existing file without truncating; if it does not exist,
     * "w+" creates it. Together this is open-or-create with no data loss —
     * Plus-FAT has no single open-always mode. */
    self->Fp = ff_fopen(path, "r+");
    if (self->Fp == NULL)
    {
        self->Fp = ff_fopen(path, "w+");
    }
    return self->Fp != NULL;
}

static void PlusFatFile_Close(struct SolidSyslogFile* base)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    if (self->Fp != NULL)
    {
        ff_fclose(self->Fp);
        self->Fp = NULL;
    }
}

static bool PlusFatFile_IsOpen(struct SolidSyslogFile* base)
{
    return PlusFatFile_SelfFromBase(base)->Fp != NULL;
}

static bool PlusFatFile_Read(struct SolidSyslogFile* base, void* buf, size_t count)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    /* xSize == 1 so the ff_fread return value is the byte count delivered. */
    return ff_fread(buf, 1, count, self->Fp) == count;
}
