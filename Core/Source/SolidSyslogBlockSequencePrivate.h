/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGBLOCKSEQUENCEPRIVATE_H
#define SOLIDSYSLOGBLOCKSEQUENCEPRIVATE_H

#include "SolidSyslogBlockStore.h"
#include "SolidSyslogBlockStoreErrors.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct SolidSyslogBlockDevice;

struct SolidSyslogBlockSequenceConfig
{
    struct SolidSyslogBlockDevice* BlockDevice;
    size_t MaxBlockSize;
    size_t MaxBlocks;
    enum SolidSyslogDiscardPolicy DiscardPolicy;
    SolidSyslogStoreThresholdFunction GetCapacityThreshold;
    SolidSyslogStoreThresholdCallback OnThresholdCrossed;
    void* ThresholdContext;
};

struct SolidSyslogBlockSequence
{
    struct SolidSyslogBlockDevice* BlockDevice;
    size_t MaxBlockSize;
    size_t MaxBlocks;
    enum SolidSyslogDiscardPolicy DiscardPolicy;
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
    size_t ReadBlockSize; /* cached once read, for a read block behind the write block */
    bool ReadBlockSizeKnown;
    size_t WritePosition;
    bool WriteBlockCorrupt;
    bool DeviceFailed;
    bool EvictedUnsent; /* the latest rotation evicted a block still holding unsent records */
    enum SolidSyslogBlockStoreErrors FailedOperation;
};

void SolidSyslogBlockSequence_Initialise(
    struct SolidSyslogBlockSequence* blockSequence,
    const struct SolidSyslogBlockSequenceConfig* config
);
void SolidSyslogBlockSequence_Cleanup(struct SolidSyslogBlockSequence* blockSequence);

struct SolidSyslogBlockSequence* SolidSyslogBlockSequence_Create(const struct SolidSyslogBlockSequenceConfig* config);
void SolidSyslogBlockSequence_Destroy(struct SolidSyslogBlockSequence* blockSequence);

bool SolidSyslogBlockSequence_Open(struct SolidSyslogBlockSequence* blockSequence);

bool SolidSyslogBlockSequence_PrepareForWrite(
    struct SolidSyslogBlockSequence* blockSequence,
    size_t recordSize,
    bool* readBlockChanged
);
struct SolidSyslogBlockDevice* SolidSyslogBlockSequence_BlockDevice(const struct SolidSyslogBlockSequence* blockSequence
);
size_t SolidSyslogBlockSequence_WriteSequence(const struct SolidSyslogBlockSequence* blockSequence);
/* How far the write block's data runs: where the next record is appended. */
size_t SolidSyslogBlockSequence_WritePosition(const struct SolidSyslogBlockSequence* blockSequence);
void SolidSyslogBlockSequence_NoteRecordWritten(struct SolidSyslogBlockSequence* blockSequence, size_t recordSize);
void SolidSyslogBlockSequence_MarkWriteBlockCorrupt(struct SolidSyslogBlockSequence* blockSequence);

size_t SolidSyslogBlockSequence_ReadSequence(const struct SolidSyslogBlockSequence* blockSequence);
size_t SolidSyslogBlockSequence_ReadCursor(const struct SolidSyslogBlockSequence* blockSequence);
void SolidSyslogBlockSequence_SetReadCursor(struct SolidSyslogBlockSequence* blockSequence, size_t cursor);
/* How far the read block's data runs: the write position when it is the write
 * block, otherwise what the device reports. False, with the failure noted, when
 * the device cannot say - never read as an empty block. */
bool SolidSyslogBlockSequence_ReadBlockSize(struct SolidSyslogBlockSequence* blockSequence, size_t* size);
void SolidSyslogBlockSequence_AdvanceToNextReadBlock(struct SolidSyslogBlockSequence* blockSequence);
bool SolidSyslogBlockSequence_ReadIsBehindWrite(const struct SolidSyslogBlockSequence* blockSequence);
void SolidSyslogBlockSequence_DisposeReadBlockIfDrained(
    struct SolidSyslogBlockSequence* blockSequence,
    bool* readBlockChanged
);

/* Set by the first block device operation that fails; the store freezes on it. */
bool SolidSyslogBlockSequence_DeviceFailed(const struct SolidSyslogBlockSequence* blockSequence);
bool SolidSyslogBlockSequence_EvictedUnsent(const struct SolidSyslogBlockSequence* blockSequence);
enum SolidSyslogBlockStoreErrors SolidSyslogBlockSequence_FailedOperation(
    const struct SolidSyslogBlockSequence* blockSequence
);

bool SolidSyslogBlockSequence_HasUnsent(const struct SolidSyslogBlockSequence* blockSequence);
bool SolidSyslogBlockSequence_IsHalted(const struct SolidSyslogBlockSequence* blockSequence);
size_t SolidSyslogBlockSequence_TotalBytes(const struct SolidSyslogBlockSequence* blockSequence);
size_t SolidSyslogBlockSequence_UsedBytes(const struct SolidSyslogBlockSequence* blockSequence);

#endif /* SOLIDSYSLOGBLOCKSEQUENCEPRIVATE_H */
