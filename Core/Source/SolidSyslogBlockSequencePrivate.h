/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGBLOCKSEQUENCEPRIVATE_H
#define SOLIDSYSLOGBLOCKSEQUENCEPRIVATE_H

#include "SolidSyslogBlockStore.h"
#include "SolidSyslogBlockStoreErrors.h"

#include <stdbool.h>
#include <stddef.h>

struct SolidSyslogBlockDevice;

struct SolidSyslogBlockSequenceConfig
{
    struct SolidSyslogBlockDevice* BlockDevice;
    size_t DeviceBlocks; /* the device's block count, where block numbers wrap */
    size_t MaxBlockSize;
    size_t MaxBlocks;
    SolidSyslogDiscardPolicyFunction GetDiscardPolicy;
    void* DiscardPolicyContext;
    SolidSyslogStoreThresholdFunction GetCapacityThreshold;
    SolidSyslogStoreThresholdCallback OnThresholdCrossed;
    void* ThresholdContext;
};

struct SolidSyslogBlockSequence
{
    struct SolidSyslogBlockDevice* BlockDevice;
    size_t DeviceBlocks; /* the device's block count, where block numbers wrap */
    size_t MaxBlockSize;
    size_t MaxBlocks;
    SolidSyslogDiscardPolicyFunction GetDiscardPolicy;
    void* DiscardPolicyContext;
    SolidSyslogStoreThresholdFunction GetCapacityThreshold;
    SolidSyslogStoreThresholdCallback OnThresholdCrossed;
    void* ThresholdContext;
    bool AtCapacity;
    bool ThresholdCrossed;
    size_t OldestSequence;
    size_t ReadSequence;
    size_t WriteSequence;
    size_t HolesAhead; /* blocks the resume found missing from inside the run, not yet stepped over */
    size_t ReadCursor;
    size_t ReadBlockSize; /* cached once read, for a read block behind the write block */
    bool ReadBlockSizeKnown;
    size_t WritePosition;
    bool WriteBlockCorrupt;
    bool DeviceFailed;
    bool EvictedUnsent; /* the latest rotation evicted a block still holding unsent records */
    bool RotationToFinish; /* a new block has been taken, and FinishRotation has not yet run for it */
    enum SolidSyslogBlockStoreErrors FailedOperation;
};

void SolidSyslogBlockSequence_Initialise(
    struct SolidSyslogBlockSequence* blockSequence,
    const struct SolidSyslogBlockSequenceConfig* config
);
void SolidSyslogBlockSequence_Cleanup(struct SolidSyslogBlockSequence* blockSequence);

struct SolidSyslogBlockSequence* SolidSyslogBlockSequence_Create(const struct SolidSyslogBlockSequenceConfig* config);
void SolidSyslogBlockSequence_Destroy(struct SolidSyslogBlockSequence* blockSequence);

/* The fewest blocks a device can have: the least MaxBlocks may be, and the
 * blocks kept free beyond it. */
size_t SolidSyslogBlockSequence_MinDeviceBlocks(void);

bool SolidSyslogBlockSequence_Open(struct SolidSyslogBlockSequence* blockSequence);

/* Makes room for a record of recordSize in the write block, taking the next
 * block where the current one is full. False where the record is refused: the
 * store is full and keeps what it has, or the device failed. A rotation it
 * makes is finished by SolidSyslogBlockSequence_FinishRotation. */
bool SolidSyslogBlockSequence_PrepareForWrite(struct SolidSyslogBlockSequence* blockSequence, size_t recordSize);
/* Does for the blocks behind the write block what the last PrepareForWrite's
 * rotation left owing, and nothing where it made none: the read block is
 * disposed of where it is drained, and the oldest block evicted where the run
 * is then still over its ceiling. True where either changed the read block.
 * Called once the record that needed the new block has been appended, because
 * a dispose can take as long as an erase and the record is not left waiting on
 * it. A dispose that fails is noted as a device failure. */
bool SolidSyslogBlockSequence_FinishRotation(struct SolidSyslogBlockSequence* blockSequence);
struct SolidSyslogBlockDevice* SolidSyslogBlockSequence_BlockDevice(const struct SolidSyslogBlockSequence* blockSequence
);
size_t SolidSyslogBlockSequence_WriteSequence(const struct SolidSyslogBlockSequence* blockSequence);
/* How far the write block's data runs: where the next record is appended. */
size_t SolidSyslogBlockSequence_WritePosition(const struct SolidSyslogBlockSequence* blockSequence);
/* A record of recordSize is on the device at the write position, which moves
 * past it. Called before FinishRotation, so the record's bytes count as used
 * while the blocks behind the write block are seen to. */
void SolidSyslogBlockSequence_NoteRecordAppended(struct SolidSyslogBlockSequence* blockSequence, size_t recordSize);
/* The store has kept the record: it is no longer refusing, and the threshold
 * is judged against what is now used. Called after FinishRotation, so a run a
 * rotation left over its ceiling is not judged before the oldest block goes. */
void SolidSyslogBlockSequence_NoteRecordKept(struct SolidSyslogBlockSequence* blockSequence);
void SolidSyslogBlockSequence_MarkWriteBlockCorrupt(struct SolidSyslogBlockSequence* blockSequence);

size_t SolidSyslogBlockSequence_ReadSequence(const struct SolidSyslogBlockSequence* blockSequence);
size_t SolidSyslogBlockSequence_ReadCursor(const struct SolidSyslogBlockSequence* blockSequence);
void SolidSyslogBlockSequence_SetReadCursor(struct SolidSyslogBlockSequence* blockSequence, size_t cursor);
/* How far the read block's data runs: the write position when it is the write
 * block, otherwise what the device reports. False, with the failure noted, when
 * the device cannot say - never read as an empty block. */
bool SolidSyslogBlockSequence_ReadBlockSize(struct SolidSyslogBlockSequence* blockSequence, size_t* size);
bool SolidSyslogBlockSequence_ReadIsBehindWrite(const struct SolidSyslogBlockSequence* blockSequence);
void SolidSyslogBlockSequence_DisposeReadBlockIfDrained(
    struct SolidSyslogBlockSequence* blockSequence,
    bool* readBlockChanged
);
/* The read is done with its block - nothing in it remains to deliver, whether
 * its records were marked, stepped past or given up - so the block is disposed
 * of and the read starts on the next. Only for a read block behind the write
 * block: the write block is never left. A dispose that fails is noted as a
 * device failure. */
void SolidSyslogBlockSequence_LeaveReadBlock(struct SolidSyslogBlockSequence* blockSequence);

/* Set by the first block device operation that fails; the store freezes on it. */
bool SolidSyslogBlockSequence_DeviceFailed(const struct SolidSyslogBlockSequence* blockSequence);
bool SolidSyslogBlockSequence_EvictedUnsent(const struct SolidSyslogBlockSequence* blockSequence);
enum SolidSyslogBlockStoreErrors SolidSyslogBlockSequence_FailedOperation(
    const struct SolidSyslogBlockSequence* blockSequence
);

bool SolidSyslogBlockSequence_HasUnsent(const struct SolidSyslogBlockSequence* blockSequence);
size_t SolidSyslogBlockSequence_TotalBytes(const struct SolidSyslogBlockSequence* blockSequence);
size_t SolidSyslogBlockSequence_UsedBytes(const struct SolidSyslogBlockSequence* blockSequence);

#endif /* SOLIDSYSLOGBLOCKSEQUENCEPRIVATE_H */
