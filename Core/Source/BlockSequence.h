#ifndef SOLIDSYSLOG_BLOCKSEQUENCE_H
#define SOLIDSYSLOG_BLOCKSEQUENCE_H

#include "SolidSyslogFile.h"
#include "SolidSyslogFileStore.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct BlockSequenceConfig
{
    struct SolidSyslogFile*       readFile;
    struct SolidSyslogFile*       writeFile;
    const char*                   pathPrefix;
    size_t                        maxFileSize;
    size_t                        maxFiles;
    enum SolidSyslogDiscardPolicy discardPolicy;
    SolidSyslogStoreFullCallback  onStoreFull;
    void*                         storeFullContext;
};

struct BlockSequence
{
    struct SolidSyslogFile*       readFile;
    struct SolidSyslogFile*       writeFile;
    const char*                   pathPrefix;
    size_t                        maxFileSize;
    size_t                        maxFiles;
    enum SolidSyslogDiscardPolicy discardPolicy;
    SolidSyslogStoreFullCallback  onStoreFull;
    void*                         storeFullContext;
    bool                          halted;
    bool                          atCapacity;
    uint8_t                       oldestSequence;
    uint8_t                       readSequence;
    uint8_t                       writeSequence;
    size_t                        readCursor;
    size_t                        writePosition;
    bool                          writeFileCorrupt;
};

void BlockSequence_Init(struct BlockSequence* blockSequence, const struct BlockSequenceConfig* config);
bool BlockSequence_Open(struct BlockSequence* blockSequence);
void BlockSequence_Close(struct BlockSequence* blockSequence);

bool                    BlockSequence_PrepareForWrite(struct BlockSequence* blockSequence, size_t recordSize, bool* readFileChanged);
struct SolidSyslogFile* BlockSequence_WriteFile(const struct BlockSequence* blockSequence);
size_t                  BlockSequence_WritePosition(const struct BlockSequence* blockSequence);
void                    BlockSequence_NoteRecordWritten(struct BlockSequence* blockSequence, size_t recordSize);
void                    BlockSequence_MarkWriteFileCorrupt(struct BlockSequence* blockSequence);

struct SolidSyslogFile* BlockSequence_ReadFile(const struct BlockSequence* blockSequence);
size_t                  BlockSequence_ReadCursor(const struct BlockSequence* blockSequence);
void                    BlockSequence_SetReadCursor(struct BlockSequence* blockSequence, size_t cursor);
void                    BlockSequence_AdvanceToNextReadFile(struct BlockSequence* blockSequence);
bool                    BlockSequence_IsReadingOlderFile(const struct BlockSequence* blockSequence);

bool   BlockSequence_HasUnsent(const struct BlockSequence* blockSequence);
bool   BlockSequence_IsHalted(const struct BlockSequence* blockSequence);
size_t BlockSequence_TotalBytes(const struct BlockSequence* blockSequence);
size_t BlockSequence_UsedBytes(const struct BlockSequence* blockSequence);

#endif /* SOLIDSYSLOG_BLOCKSEQUENCE_H */
