#ifndef SOLIDSYSLOG_BLOCKSEQUENCEPRIVATE_H
#define SOLIDSYSLOG_BLOCKSEQUENCEPRIVATE_H

#include "SolidSyslogBlockStore.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct SolidSyslogBlockDevice;

struct BlockSequenceConfig
{
    struct SolidSyslogBlockDevice* BlockDevice;
    size_t MaxBlockSize;
    size_t MaxBlocks;
    enum SolidSyslogDiscardPolicy DiscardPolicy;
    SolidSyslogStoreFullCallback OnStoreFull;
    void* StoreFullContext;
    SolidSyslogStoreThresholdFunction GetCapacityThreshold;
    SolidSyslogStoreThresholdCallback OnThresholdCrossed;
    void* ThresholdContext;
};

struct BlockSequence
{
    struct SolidSyslogBlockDevice* BlockDevice;
    size_t MaxBlockSize;
    size_t MaxBlocks;
    enum SolidSyslogDiscardPolicy DiscardPolicy;
    SolidSyslogStoreFullCallback OnStoreFull;
    void* StoreFullContext;
    SolidSyslogStoreThresholdFunction GetCapacityThreshold;
    SolidSyslogStoreThresholdCallback OnThresholdCrossed;
    void* ThresholdContext;
    bool Halted;
    bool AtCapacity;
    bool ThresholdCrossed;
    uint8_t OldestSequence;
    uint8_t ReadSequence;
    uint8_t WriteSequence;
    size_t ReadCursor;
    size_t WritePosition;
    bool WriteBlockCorrupt;
};

void BlockSequence_Initialise(struct BlockSequence* blockSequence, const struct BlockSequenceConfig* config);
void BlockSequence_Cleanup(struct BlockSequence* blockSequence);

struct BlockSequence* BlockSequence_Create(const struct BlockSequenceConfig* config);
void BlockSequence_Destroy(struct BlockSequence* blockSequence);

bool BlockSequence_Open(struct BlockSequence* blockSequence);

bool BlockSequence_PrepareForWrite(struct BlockSequence* blockSequence, size_t recordSize, bool* readBlockChanged);
struct SolidSyslogBlockDevice* BlockSequence_BlockDevice(const struct BlockSequence* blockSequence);
size_t BlockSequence_WriteSequence(const struct BlockSequence* blockSequence);
void BlockSequence_NoteRecordWritten(struct BlockSequence* blockSequence, size_t recordSize);
void BlockSequence_MarkWriteBlockCorrupt(struct BlockSequence* blockSequence);

size_t BlockSequence_ReadSequence(const struct BlockSequence* blockSequence);
size_t BlockSequence_ReadCursor(const struct BlockSequence* blockSequence);
void BlockSequence_SetReadCursor(struct BlockSequence* blockSequence, size_t cursor);
void BlockSequence_AdvanceToNextReadBlock(struct BlockSequence* blockSequence);
bool BlockSequence_ReadIsBehindWrite(const struct BlockSequence* blockSequence);
void BlockSequence_DisposeReadBlockIfDrained(struct BlockSequence* blockSequence, bool* readBlockChanged);

bool BlockSequence_HasUnsent(const struct BlockSequence* blockSequence);
bool BlockSequence_IsHalted(const struct BlockSequence* blockSequence);
size_t BlockSequence_TotalBytes(const struct BlockSequence* blockSequence);
size_t BlockSequence_UsedBytes(const struct BlockSequence* blockSequence);

#endif /* SOLIDSYSLOG_BLOCKSEQUENCEPRIVATE_H */
