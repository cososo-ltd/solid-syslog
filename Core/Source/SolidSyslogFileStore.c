#include "SolidSyslogFileStore.h"
#include "BlockSequence.h"
#include "RecordStore.h"
#include "SolidSyslog.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogStoreDefinition.h"

/* vtable — forward-declared because InitialiseVtable references them before their definitions */
static bool Write(struct SolidSyslogStore* self, const void* data, size_t size);
static bool ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead);
static void MarkSent(struct SolidSyslogStore* self);
static bool HasUnsent(struct SolidSyslogStore* self);
static bool IsHalted(struct SolidSyslogStore* self);

/* ------------------------------------------------------------------
 * Instance
 * ----------------------------------------------------------------*/

struct SolidSyslogFileStore
{
    struct SolidSyslogStore base;
    struct RecordStore      recordStore;
    struct BlockSequence    blockSequence;
};

static const struct SolidSyslogFileStore DEFAULT_INSTANCE = {0};
static struct SolidSyslogFileStore       instance;

static inline struct SolidSyslogSecurityPolicy* ResolveSecurityPolicy(struct SolidSyslogSecurityPolicy* configured);
static inline void                              InitialiseVtable(void);
static void                                     ResumeFromExistingFile(void);

/* ------------------------------------------------------------------
 * Create
 * ----------------------------------------------------------------*/

struct SolidSyslogStore* SolidSyslogFileStore_Create(const struct SolidSyslogFileStoreConfig* config)
{
    instance = DEFAULT_INSTANCE;

    struct SolidSyslogSecurityPolicy* securityPolicy = ResolveSecurityPolicy(config->securityPolicy);
    RecordStore_Init(&instance.recordStore, securityPolicy);

    struct BlockSequenceConfig blockConfig = {
        .readFile      = config->readFile,
        .writeFile     = config->writeFile,
        .pathPrefix    = config->pathPrefix,
        .maxFileSize   = (config->maxFileSize < RecordStore_RecordSize(&instance.recordStore, SOLIDSYSLOG_MAX_MESSAGE_SIZE))
                             ? RecordStore_RecordSize(&instance.recordStore, SOLIDSYSLOG_MAX_MESSAGE_SIZE)
                             : config->maxFileSize,
        .maxFiles      = config->maxFiles,
        .discardPolicy = config->discardPolicy,
        .onStoreFull   = config->onStoreFull,
    };
    BlockSequence_Init(&instance.blockSequence, &blockConfig);

    InitialiseVtable();

    if (BlockSequence_Open(&instance.blockSequence))
    {
        ResumeFromExistingFile();
    }

    return &instance.base;
}

static inline struct SolidSyslogSecurityPolicy* ResolveSecurityPolicy(struct SolidSyslogSecurityPolicy* configured)
{
    if ((configured == NULL) || (configured->integritySize > SOLIDSYSLOG_MAX_INTEGRITY_SIZE))
    {
        return SolidSyslogNullSecurityPolicy_Create();
    }

    return configured;
}

static inline void InitialiseVtable(void)
{
    instance.base.Write          = Write;
    instance.base.ReadNextUnsent = ReadNextUnsent;
    instance.base.MarkSent       = MarkSent;
    instance.base.HasUnsent      = HasUnsent;
    instance.base.IsHalted       = IsHalted;
}

static void ResumeFromExistingFile(void)
{
    bool   corrupt = false;
    size_t cursor  = RecordStore_FindFirstUnsent(&instance.recordStore, BlockSequence_ReadFile(&instance.blockSequence),
                                                 BlockSequence_WritePosition(&instance.blockSequence), &corrupt);

    BlockSequence_SetReadCursor(&instance.blockSequence, cursor);

    if (corrupt)
    {
        BlockSequence_MarkWriteFileCorrupt(&instance.blockSequence);
    }
}

/* ------------------------------------------------------------------
 * Destroy
 * ----------------------------------------------------------------*/

void SolidSyslogFileStore_Destroy(void)
{
    BlockSequence_Close(&instance.blockSequence);
    instance = DEFAULT_INSTANCE;
}

/* ------------------------------------------------------------------
 * Write
 * ----------------------------------------------------------------*/

static bool StoreRecord(const void* data, size_t size);

static bool Write(struct SolidSyslogStore* self, const void* data, size_t size)
{
    (void) self;
    bool written = false;

    if (SolidSyslogFile_IsOpen(BlockSequence_WriteFile(&instance.blockSequence)))
    {
        written = StoreRecord(data, size);
    }

    return written;
}

static bool StoreRecord(const void* data, size_t size)
{
    size_t recordSize      = RecordStore_RecordSize(&instance.recordStore, (uint16_t) size);
    bool   readFileChanged = false;
    bool   written         = false;

    if (BlockSequence_PrepareForWrite(&instance.blockSequence, recordSize, &readFileChanged))
    {
        if (readFileChanged)
        {
            RecordStore_ForgetLastRead(&instance.recordStore);
        }

        if (RecordStore_Append(&instance.recordStore, BlockSequence_WriteFile(&instance.blockSequence), BlockSequence_WritePosition(&instance.blockSequence),
                               data, size))
        {
            BlockSequence_NoteRecordWritten(&instance.blockSequence, recordSize);
            written = true;
        }
    }

    return written;
}

/* ------------------------------------------------------------------
 * HasUnsent / IsHalted
 * ----------------------------------------------------------------*/

static bool HasUnsent(struct SolidSyslogStore* self)
{
    (void) self;
    return BlockSequence_HasUnsent(&instance.blockSequence);
}

static bool IsHalted(struct SolidSyslogStore* self)
{
    (void) self;
    return BlockSequence_IsHalted(&instance.blockSequence);
}

/* ------------------------------------------------------------------
 * ReadNextUnsent
 * ----------------------------------------------------------------*/

static bool ReadCurrent(void* data, size_t maxSize, size_t* bytesRead);

static bool ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead)
{
    (void) self;
    bool read  = false;
    *bytesRead = 0;

    if (BlockSequence_HasUnsent(&instance.blockSequence))
    {
        read = ReadCurrent(data, maxSize, bytesRead);

        while (!read && BlockSequence_ReadingOlderFile(&instance.blockSequence))
        {
            BlockSequence_AdvanceToNextReadFile(&instance.blockSequence);
            RecordStore_ForgetLastRead(&instance.recordStore);
            read = ReadCurrent(data, maxSize, bytesRead);
        }
    }

    return read;
}

static bool ReadCurrent(void* data, size_t maxSize, size_t* bytesRead)
{
    return RecordStore_Read(&instance.recordStore, BlockSequence_ReadFile(&instance.blockSequence), BlockSequence_ReadCursor(&instance.blockSequence), data,
                            maxSize, bytesRead);
}

/* ------------------------------------------------------------------
 * MarkSent
 * ----------------------------------------------------------------*/

static void MarkSent(struct SolidSyslogStore* self)
{
    (void) self;

    size_t nextCursor = 0;

    if (RecordStore_MarkLastReadAsSent(&instance.recordStore, BlockSequence_ReadFile(&instance.blockSequence), &nextCursor))
    {
        BlockSequence_SetReadCursor(&instance.blockSequence, nextCursor);
    }
}
