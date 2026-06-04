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
static bool PlusFatFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count);
static void PlusFatFile_SeekTo(struct SolidSyslogFile* base, size_t offset);
static size_t PlusFatFile_Size(struct SolidSyslogFile* base);
static void PlusFatFile_Truncate(struct SolidSyslogFile* base);
static bool PlusFatFile_Exists(struct SolidSyslogFile* base, const char* path);
static bool PlusFatFile_Delete(struct SolidSyslogFile* base, const char* path);

static inline struct SolidSyslogPlusFatFile* PlusFatFile_SelfFromBase(struct SolidSyslogFile* base);
static inline bool PlusFatFile_IsFileOpen(const struct SolidSyslogPlusFatFile* self);
static inline bool PlusFatFile_Flush(struct SolidSyslogPlusFatFile* self);

void PlusFatFile_Initialise(struct SolidSyslogFile* base)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    self->Base.Open = PlusFatFile_Open;
    self->Base.Close = PlusFatFile_Close;
    self->Base.IsOpen = PlusFatFile_IsOpen;
    self->Base.Read = PlusFatFile_Read;
    self->Base.Write = PlusFatFile_Write;
    self->Base.SeekTo = PlusFatFile_SeekTo;
    self->Base.Size = PlusFatFile_Size;
    self->Base.Truncate = PlusFatFile_Truncate;
    self->Base.Exists = PlusFatFile_Exists;
    self->Base.Delete = PlusFatFile_Delete;
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

static void PlusFatFile_Close(struct SolidSyslogFile* base)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    if (PlusFatFile_IsFileOpen(self))
    {
        ff_fclose(self->Fp);
        self->Fp = NULL;
    }
}

/* Open-state is carried by the FF_FILE* sentinel: non-NULL once a file is open,
 * NULL when closed. The single source of truth for every open-state check. */
static inline bool PlusFatFile_IsFileOpen(const struct SolidSyslogPlusFatFile* self)
{
    return self->Fp != NULL;
}

static bool PlusFatFile_Open(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    /* "r+" opens an existing file without truncating; if it does not exist,
     * "w+" creates it. Together this is open-or-create with no data loss —
     * Plus-FAT has no single open-always mode. */
    self->Fp = ff_fopen(path, "r+");
    if (!PlusFatFile_IsFileOpen(self))
    {
        self->Fp = ff_fopen(path, "w+");
    }
    return PlusFatFile_IsFileOpen(self);
}

static bool PlusFatFile_IsOpen(struct SolidSyslogFile* base)
{
    return PlusFatFile_IsFileOpen(PlusFatFile_SelfFromBase(base));
}

static bool PlusFatFile_Read(struct SolidSyslogFile* base, void* buf, size_t count)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    /* xSize == 1 so the ff_fread return value is the byte count delivered. */
    return ff_fread(buf, 1, count, self->Fp) == count;
}

static bool PlusFatFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    /* Flush the IO-manager cache after every complete write so a power loss
     * never loses a record the BlockStore was told had been stored. Plus-FAT
     * has no per-file flush — ff_stdio.h declares ff_fflush but the library
     * never defines it; FF_FlushCache against the file's IO manager is the real
     * durability primitive. The directory entry's size is committed on Close. */
    bool wroteAll = (ff_fwrite(buf, 1, count, self->Fp) == count);
    return wroteAll && PlusFatFile_Flush(self);
}

/* Flush the IO-manager cache to the media; returns whether it succeeded. */
static inline bool PlusFatFile_Flush(struct SolidSyslogPlusFatFile* self)
{
    return FF_FlushCache(self->Fp->pxIOManager) == FF_ERR_NONE;
}

static void PlusFatFile_SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    ff_fseek(self->Fp, (long) offset, SEEK_SET);
}

static size_t PlusFatFile_Size(struct SolidSyslogFile* base)
{
    return ff_filelength(PlusFatFile_SelfFromBase(base)->Fp);
}

static void PlusFatFile_Truncate(struct SolidSyslogFile* base)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    /* Empty the file: rewind to the start, then set EOF there so the length
     * becomes zero. ff_seteof truncates at the current position and Plus-FAT
     * has no truncate-to-zero call of its own. */
    ff_fseek(self->Fp, 0, SEEK_SET);
    ff_seteof(self->Fp);
}

static bool PlusFatFile_Exists(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    FF_Stat_t status;
    return ff_stat(path, &status) == 0;
}

static bool PlusFatFile_Delete(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    return ff_remove(path) == 0;
}
