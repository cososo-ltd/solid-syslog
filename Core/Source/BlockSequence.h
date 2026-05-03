#ifndef SOLIDSYSLOG_BLOCKSEQUENCE_H
#define SOLIDSYSLOG_BLOCKSEQUENCE_H

#include "SolidSyslogFileStore.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct SolidSyslogBlockDevice;

struct BlockSequenceConfig
{
    struct SolidSyslogBlockDevice*    blockDevice;
    size_t                            maxBlockSize;
    size_t                            maxBlocks;
    enum SolidSyslogDiscardPolicy     discardPolicy;
    SolidSyslogStoreFullCallback      onStoreFull;
    void*                             storeFullContext;
    SolidSyslogStoreThresholdFunction getCapacityThreshold;
    SolidSyslogStoreThresholdCallback onThresholdCrossed;
    void*                             thresholdContext;
};

struct BlockSequence
{
    struct SolidSyslogBlockDevice*    blockDevice;
    size_t                            maxBlockSize;
    size_t                            maxBlocks;
    enum SolidSyslogDiscardPolicy     discardPolicy;
    SolidSyslogStoreFullCallback      onStoreFull;
    void*                             storeFullContext;
    SolidSyslogStoreThresholdFunction getCapacityThreshold;
    SolidSyslogStoreThresholdCallback onThresholdCrossed;
    void*                             thresholdContext;
    bool                              halted;
    bool                              atCapacity;
    bool                              thresholdCrossed;
    uint8_t                           oldestSequence;
    uint8_t                           readSequence;
    uint8_t                           writeSequence;
    size_t                            readCursor;
    size_t                            writePosition;
    bool                              writeBlockCorrupt;
};

void BlockSequence_Init(struct BlockSequence* blockSequence, const struct BlockSequenceConfig* config);
bool BlockSequence_Open(struct BlockSequence* blockSequence);

bool                           BlockSequence_PrepareForWrite(struct BlockSequence* blockSequence, size_t recordSize, bool* readBlockChanged);
struct SolidSyslogBlockDevice* BlockSequence_BlockDevice(const struct BlockSequence* blockSequence);
size_t                         BlockSequence_WriteSequence(const struct BlockSequence* blockSequence);
size_t                         BlockSequence_WritePosition(const struct BlockSequence* blockSequence);
void                           BlockSequence_NoteRecordWritten(struct BlockSequence* blockSequence, size_t recordSize);
void                           BlockSequence_MarkWriteBlockCorrupt(struct BlockSequence* blockSequence);

size_t BlockSequence_ReadSequence(const struct BlockSequence* blockSequence);
size_t BlockSequence_ReadCursor(const struct BlockSequence* blockSequence);
void   BlockSequence_SetReadCursor(struct BlockSequence* blockSequence, size_t cursor);
void   BlockSequence_AdvanceToNextReadBlock(struct BlockSequence* blockSequence);
bool   BlockSequence_ReadIsBehindWrite(const struct BlockSequence* blockSequence);

bool   BlockSequence_HasUnsent(const struct BlockSequence* blockSequence);
bool   BlockSequence_IsHalted(const struct BlockSequence* blockSequence);
size_t BlockSequence_TotalBytes(const struct BlockSequence* blockSequence);
size_t BlockSequence_UsedBytes(const struct BlockSequence* blockSequence);

#endif /* SOLIDSYSLOG_BLOCKSEQUENCE_H */
