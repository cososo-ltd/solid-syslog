#include "BlockSequence.h"

#include "SolidSyslogFormatter.h"

enum
{
    MIN_MAX_FILES    = 2,
    MAX_MAX_FILES    = 99,
    SEQUENCE_MODULUS = 100,
    MAX_PATH_SIZE    = 128
};

static const char FILE_EXTENSION[] = ".log";

enum
{
    SEQUENCE_DIGITS   = 2,
    FILENAME_SUFFIX   = SEQUENCE_DIGITS + sizeof(FILE_EXTENSION) - 1,
    MAX_PREFIX_LENGTH = MAX_PATH_SIZE - FILENAME_SUFFIX - 1
};

static inline const char* FormatFilename(const struct BlockSequence* blockSequence, SolidSyslogFormatterStorage* storage, uint8_t sequence)
{
    struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(storage, MAX_PATH_SIZE);

    SolidSyslogFormatter_BoundedString(f, blockSequence->pathPrefix, MAX_PREFIX_LENGTH);
    SolidSyslogFormatter_TwoDigit(f, sequence);
    SolidSyslogFormatter_BoundedString(f, FILE_EXTENSION, sizeof(FILE_EXTENSION) - 1);

    return SolidSyslogFormatter_AsFormattedBuffer(f);
}

static inline uint8_t NextSequence(uint8_t current)
{
    return (uint8_t) ((current + 1) % SEQUENCE_MODULUS);
}

static inline size_t FileCount(const struct BlockSequence* blockSequence)
{
    return (size_t) ((blockSequence->writeSequence - blockSequence->oldestSequence + SEQUENCE_MODULUS) % SEQUENCE_MODULUS) + 1;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- value, min, max have distinct semantics
static inline size_t ClampToRange(size_t value, size_t min, size_t max)
{
    size_t result = value;

    if (result < min)
    {
        result = min;
    }

    if (result > max)
    {
        result = max;
    }

    return result;
}

void BlockSequence_Init(struct BlockSequence* blockSequence, const struct BlockSequenceConfig* config)
{
    blockSequence->readFile         = config->readFile;
    blockSequence->writeFile        = config->writeFile;
    blockSequence->pathPrefix       = config->pathPrefix;
    blockSequence->maxFileSize      = config->maxFileSize;
    blockSequence->maxFiles         = ClampToRange(config->maxFiles, MIN_MAX_FILES, MAX_MAX_FILES);
    blockSequence->discardPolicy    = config->discardPolicy;
    blockSequence->onStoreFull      = config->onStoreFull;
    blockSequence->storeFullContext = config->storeFullContext;
    blockSequence->halted           = false;
    blockSequence->atCapacity       = false;
    blockSequence->oldestSequence   = 0;
    blockSequence->readSequence     = 0;
    blockSequence->writeSequence    = 0;
    blockSequence->readCursor       = 0;
    blockSequence->writePosition    = 0;
    blockSequence->writeFileCorrupt = false;
}

static void ScanForExistingFiles(struct BlockSequence* blockSequence);

bool BlockSequence_Open(struct BlockSequence* blockSequence)
{
    ScanForExistingFiles(blockSequence);

    SolidSyslogFormatterStorage writeNameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
    const char*                 writeName = FormatFilename(blockSequence, writeNameStorage, blockSequence->writeSequence);

    bool opened = SolidSyslogFile_Open(blockSequence->writeFile, writeName);

    if (opened)
    {
        SolidSyslogFormatterStorage readNameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
        const char*                 readName = FormatFilename(blockSequence, readNameStorage, blockSequence->readSequence);
        SolidSyslogFile_Open(blockSequence->readFile, readName);

        blockSequence->writePosition = SolidSyslogFile_Size(blockSequence->writeFile);
    }

    return opened;
}

static void ScanForExistingFiles(struct BlockSequence* blockSequence)
{
    enum
    {
        MAX_SEQUENCE = 100
    };

    bool foundFirst = false;

    for (int seq = 0; seq < MAX_SEQUENCE; seq++)
    {
        SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
        const char*                 name = FormatFilename(blockSequence, nameStorage, (uint8_t) seq);

        if (SolidSyslogFile_Exists(blockSequence->writeFile, name))
        {
            if (!foundFirst)
            {
                blockSequence->oldestSequence = (uint8_t) seq;
                blockSequence->readSequence   = (uint8_t) seq;
                foundFirst                    = true;
            }

            blockSequence->writeSequence = (uint8_t) seq;
        }
    }
}

static inline bool IsWriteFileOpen(const struct BlockSequence* blockSequence)
{
    return (blockSequence->writeFile != NULL) && SolidSyslogFile_IsOpen(blockSequence->writeFile);
}

static inline bool IsReadFileOpen(const struct BlockSequence* blockSequence)
{
    return (blockSequence->readFile != NULL) && SolidSyslogFile_IsOpen(blockSequence->readFile);
}

void BlockSequence_Close(struct BlockSequence* blockSequence)
{
    if (IsWriteFileOpen(blockSequence))
    {
        SolidSyslogFile_Close(blockSequence->writeFile);
    }

    if (IsReadFileOpen(blockSequence))
    {
        SolidSyslogFile_Close(blockSequence->readFile);
    }
}

static inline bool FileIsFull(const struct BlockSequence* blockSequence, size_t recordSize);
static inline bool StoreIsFull(const struct BlockSequence* blockSequence);
static inline void NotifyStoreFull(struct BlockSequence* blockSequence);
static bool        RotateToNextFile(struct BlockSequence* blockSequence);

bool BlockSequence_PrepareForWrite(struct BlockSequence* blockSequence, size_t recordSize, bool* readFileChanged)
{
    bool spaceAvailable = true;
    *readFileChanged    = false;

    if (FileIsFull(blockSequence, recordSize) && StoreIsFull(blockSequence))
    {
        NotifyStoreFull(blockSequence);
        spaceAvailable = false;
    }
    else if (FileIsFull(blockSequence, recordSize))
    {
        *readFileChanged = RotateToNextFile(blockSequence);
    }

    return spaceAvailable;
}

static inline bool FileIsFull(const struct BlockSequence* blockSequence, size_t recordSize)
{
    return blockSequence->writeFileCorrupt || (blockSequence->writePosition + recordSize) > blockSequence->maxFileSize;
}

static inline bool StoreIsFull(const struct BlockSequence* blockSequence)
{
    return (FileCount(blockSequence) >= blockSequence->maxFiles) && (blockSequence->discardPolicy != SOLIDSYSLOG_DISCARD_OLDEST);
}

static inline void NotifyStoreFull(struct BlockSequence* blockSequence)
{
    blockSequence->atCapacity = true;

    if (blockSequence->discardPolicy == SOLIDSYSLOG_HALT)
    {
        blockSequence->halted = true;

        if (blockSequence->onStoreFull != NULL)
        {
            blockSequence->onStoreFull(blockSequence->storeFullContext);
        }
    }
}

static bool        DiscardOldestFile(struct BlockSequence* blockSequence);
static void        DeleteOldestFile(struct BlockSequence* blockSequence);
static void        ResetReadToOldestFile(struct BlockSequence* blockSequence);
static inline void SwitchReadFile(struct BlockSequence* blockSequence, uint8_t newSequence);

static bool RotateToNextFile(struct BlockSequence* blockSequence)
{
    SolidSyslogFile_Close(blockSequence->writeFile);
    blockSequence->writeSequence    = NextSequence(blockSequence->writeSequence);
    blockSequence->writePosition    = 0;
    blockSequence->writeFileCorrupt = false;
    SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
    const char*                 name = FormatFilename(blockSequence, nameStorage, blockSequence->writeSequence);
    SolidSyslogFile_Open(blockSequence->writeFile, name);

    bool readFileChanged = false;

    if (FileCount(blockSequence) > blockSequence->maxFiles)
    {
        readFileChanged = DiscardOldestFile(blockSequence);
    }

    return readFileChanged;
}

static bool DiscardOldestFile(struct BlockSequence* blockSequence)
{
    bool readingOldestFile = (blockSequence->readSequence == blockSequence->oldestSequence);

    DeleteOldestFile(blockSequence);

    if (readingOldestFile)
    {
        ResetReadToOldestFile(blockSequence);
    }

    return readingOldestFile;
}

static void DeleteOldestFile(struct BlockSequence* blockSequence)
{
    SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
    const char*                 name = FormatFilename(blockSequence, nameStorage, blockSequence->oldestSequence);
    SolidSyslogFile_Delete(blockSequence->writeFile, name);
    blockSequence->oldestSequence = NextSequence(blockSequence->oldestSequence);
}

static void ResetReadToOldestFile(struct BlockSequence* blockSequence)
{
    SwitchReadFile(blockSequence, blockSequence->oldestSequence);
}

static inline void SwitchReadFile(struct BlockSequence* blockSequence, uint8_t newSequence)
{
    SolidSyslogFile_Close(blockSequence->readFile);
    blockSequence->readSequence = newSequence;
    blockSequence->readCursor   = 0;
    SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
    const char*                 name = FormatFilename(blockSequence, nameStorage, blockSequence->readSequence);
    SolidSyslogFile_Open(blockSequence->readFile, name);
}

struct SolidSyslogFile* BlockSequence_WriteFile(const struct BlockSequence* blockSequence)
{
    return blockSequence->writeFile;
}

size_t BlockSequence_WritePosition(const struct BlockSequence* blockSequence)
{
    return blockSequence->writePosition;
}

void BlockSequence_NoteRecordWritten(struct BlockSequence* blockSequence, size_t recordSize)
{
    blockSequence->writePosition += recordSize;
}

void BlockSequence_MarkWriteFileCorrupt(struct BlockSequence* blockSequence)
{
    blockSequence->writeFileCorrupt = true;
}

struct SolidSyslogFile* BlockSequence_ReadFile(const struct BlockSequence* blockSequence)
{
    return blockSequence->readFile;
}

size_t BlockSequence_ReadCursor(const struct BlockSequence* blockSequence)
{
    return blockSequence->readCursor;
}

void BlockSequence_SetReadCursor(struct BlockSequence* blockSequence, size_t cursor)
{
    blockSequence->readCursor = cursor;
}

void BlockSequence_AdvanceToNextReadFile(struct BlockSequence* blockSequence)
{
    SwitchReadFile(blockSequence, NextSequence(blockSequence->readSequence));
}

bool BlockSequence_IsReadingOlderFile(const struct BlockSequence* blockSequence)
{
    return blockSequence->readSequence != blockSequence->writeSequence;
}

bool BlockSequence_HasUnsent(const struct BlockSequence* blockSequence)
{
    return BlockSequence_IsReadingOlderFile(blockSequence) || (blockSequence->readCursor < blockSequence->writePosition);
}

bool BlockSequence_IsHalted(const struct BlockSequence* blockSequence)
{
    return blockSequence->halted;
}

size_t BlockSequence_TotalBytes(const struct BlockSequence* blockSequence)
{
    return blockSequence->maxFiles * blockSequence->maxFileSize;
}

size_t BlockSequence_UsedBytes(const struct BlockSequence* blockSequence)
{
    if (blockSequence->atCapacity)
    {
        return BlockSequence_TotalBytes(blockSequence);
    }

    size_t closedBlocks = FileCount(blockSequence) - 1;
    return (closedBlocks * blockSequence->maxFileSize) + blockSequence->writePosition;
}
