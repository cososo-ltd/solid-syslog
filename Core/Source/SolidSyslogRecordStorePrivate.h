/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGRECORDSTOREPRIVATE_H
#define SOLIDSYSLOGRECORDSTOREPRIVATE_H

#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct SolidSyslogBlockDevice;

enum
{
    RECORD_BUFFER_SIZE = 2U + 2U + SOLIDSYSLOG_MAX_MESSAGE_SIZE + SOLIDSYSLOG_MAX_INTEGRITY_SIZE + 1U
};

struct SolidSyslogRecordStore
{
    struct SolidSyslogSecurityPolicy* SecurityPolicy;
    bool HasReadRecord;
    size_t LastReadBlockIndex;
    size_t LastSentFlagOffset;
    uint8_t Buffer[RECORD_BUFFER_SIZE];
};

void SolidSyslogRecordStore_Initialise(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogSecurityPolicy* securityPolicy
);
void SolidSyslogRecordStore_Cleanup(struct SolidSyslogRecordStore* recordStore);

struct SolidSyslogRecordStore* SolidSyslogRecordStore_Create(struct SolidSyslogSecurityPolicy* securityPolicy);
void SolidSyslogRecordStore_Destroy(struct SolidSyslogRecordStore* recordStore);

size_t SolidSyslogRecordStore_RecordSize(const struct SolidSyslogRecordStore* recordStore, uint16_t dataLength);

/* What Append did: the policy may refuse to seal before the device is asked. */
enum SolidSyslogRecordStoreAppendResult
{
    SOLIDSYSLOG_RECORD_STORE_APPEND_SUCCEEDED,
    SOLIDSYSLOG_RECORD_STORE_APPEND_SEAL_FAILED,
    SOLIDSYSLOG_RECORD_STORE_APPEND_DEVICE_FAILED
};

enum SolidSyslogRecordStoreAppendResult SolidSyslogRecordStore_Append(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    const void* data,
    size_t dataSize
);

/* What Read found. A record that runs past the end of the block's data, or that
 * fails validation, is no record; a device read that fails within the data is
 * a device failure. A record the policy rejects has a valid header, so its
 * extent is known and it can be stepped over. */
enum SolidSyslogRecordStoreReadResult
{
    SOLIDSYSLOG_RECORD_STORE_READ_SUCCEEDED,
    SOLIDSYSLOG_RECORD_STORE_READ_NO_RECORD,
    SOLIDSYSLOG_RECORD_STORE_READ_REJECTED,
    SOLIDSYSLOG_RECORD_STORE_READ_DEVICE_FAILED
};

/* @p dataSize is how far the block's data runs: a span past it is no record. */
enum SolidSyslogRecordStoreReadResult SolidSyslogRecordStore_Read(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    size_t dataSize,
    void* dst,
    size_t maxSize,
    size_t* bytesRead
);

/* What MarkLastReadAsSent did: nothing when no record has been read since the
 * last mark. */
enum SolidSyslogRecordStoreMarkResult
{
    SOLIDSYSLOG_RECORD_STORE_MARK_SUCCEEDED,
    SOLIDSYSLOG_RECORD_STORE_MARK_NOTHING_READ,
    SOLIDSYSLOG_RECORD_STORE_MARK_DEVICE_FAILED
};

enum SolidSyslogRecordStoreMarkResult SolidSyslogRecordStore_MarkLastReadAsSent(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t* nextCursor
);

void SolidSyslogRecordStore_ForgetLastRead(struct SolidSyslogRecordStore* recordStore);

/* Marks the record Read last met as discarded, so no later read or resume scan
 * offers it again: @p nextCursor is where the record after it starts. */
enum SolidSyslogRecordStoreMarkResult SolidSyslogRecordStore_DiscardLastRead(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t* nextCursor
);

/* What FindFirstUnsent met on its way through the block. */
enum SolidSyslogRecordStoreScanResult
{
    SOLIDSYSLOG_RECORD_STORE_SCAN_CLEAN,
    SOLIDSYSLOG_RECORD_STORE_SCAN_CORRUPT,
    SOLIDSYSLOG_RECORD_STORE_SCAN_DEVICE_FAILED
};

size_t SolidSyslogRecordStore_FindFirstUnsent(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t blockSize,
    enum SolidSyslogRecordStoreScanResult* scanned
);

#endif /* SOLIDSYSLOGRECORDSTOREPRIVATE_H */
