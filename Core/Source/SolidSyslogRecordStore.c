/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogRecordStorePrivate.h"

#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

struct SolidSyslogBlockDevice;

enum
{
    MAGIC_SIZE = 2,
    MAGIC_BYTE_0 = 0xA5,
    MAGIC_BYTE_1 = 0x5A,
    RECORD_LENGTH_SIZE = 2,
    /* The policy authenticates magic|length as the cleartext header (an AEAD
     * policy never encrypts it) and message as the body. */
    CONTENT_HEADER_SIZE = MAGIC_SIZE + RECORD_LENGTH_SIZE,
    SENT_FLAG_SIZE = 1,
    SENT_FLAG_UNSENT = 0xFF,
    SENT_FLAG_SENT = 0x00
};

/* Each record in the buffer is laid out as
 *   [ magic | length | message | trailer | sentFlag ]
 * The security policy authenticates the content span (magic|length|message),
 * treating magic|length as the header and message as the body, and owns the
 * trailer (TrailerSize bytes). Field-address helpers chain outward from the
 * record's base so the field offsets are stated in one place each. The same
 * chaining shapes the block-offset helpers below. */

static inline uint8_t* RecordStore_MagicAddress(struct SolidSyslogRecordStore* recordStore)
{
    return recordStore->Buffer;
}

static inline uint8_t* RecordStore_LengthAddress(struct SolidSyslogRecordStore* recordStore)
{
    uint8_t* magic = RecordStore_MagicAddress(recordStore);
    return &magic[MAGIC_SIZE];
}

static inline uint8_t* RecordStore_MessageAddress(struct SolidSyslogRecordStore* recordStore)
{
    uint8_t* length = RecordStore_LengthAddress(recordStore);
    return &length[RECORD_LENGTH_SIZE];
}

static inline uint8_t* RecordStore_TrailerAddress(struct SolidSyslogRecordStore* recordStore, size_t dataSize)
{
    uint8_t* message = RecordStore_MessageAddress(recordStore);
    return &message[dataSize];
}

static inline uint8_t* RecordStore_SentFlagAddress(struct SolidSyslogRecordStore* recordStore, size_t dataSize)
{
    uint8_t* trailer = RecordStore_TrailerAddress(recordStore, dataSize);
    return &trailer[recordStore->SecurityPolicy->TrailerSize];
}

static inline uint8_t* RecordStore_ContentAddress(struct SolidSyslogRecordStore* recordStore)
{
    return RecordStore_MagicAddress(recordStore);
}

static inline uint16_t RecordStore_ContentSize(size_t dataSize)
{
    return (uint16_t) (MAGIC_SIZE + RECORD_LENGTH_SIZE + dataSize);
}

static inline size_t RecordStore_TrailerOffset(size_t recordStart, uint16_t dataLength)
{
    return recordStart + MAGIC_SIZE + RECORD_LENGTH_SIZE + dataLength;
}

static inline size_t RecordStore_SentFlagOffset(
    const struct SolidSyslogRecordStore* recordStore,
    size_t recordStart,
    uint16_t dataLength
)
{
    return RecordStore_TrailerOffset(recordStart, dataLength) + recordStore->SecurityPolicy->TrailerSize;
}

void SolidSyslogRecordStore_Initialise(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogSecurityPolicy* securityPolicy
)
{
    recordStore->SecurityPolicy = securityPolicy;
    recordStore->HasReadRecord = false;
    recordStore->LastReadBlockIndex = 0;
    recordStore->LastSentFlagOffset = 0;
}

void SolidSyslogRecordStore_Cleanup(struct SolidSyslogRecordStore* recordStore)
{
    /* No owned resources to release. The next SolidSyslogRecordStore_Initialise overwrites every
     * field, so leave the slot's bytes alone - clearing them would be
     * write-then-overwrite churn. */
    (void) recordStore;
}

size_t SolidSyslogRecordStore_RecordSize(const struct SolidSyslogRecordStore* recordStore, uint16_t dataLength)
{
    return (size_t) MAGIC_SIZE + RECORD_LENGTH_SIZE + dataLength + recordStore->SecurityPolicy->TrailerSize +
           SENT_FLAG_SIZE;
}

static inline bool RecordStore_AssembleRecord(
    struct SolidSyslogRecordStore* recordStore,
    const void* data,
    size_t size
);

enum SolidSyslogRecordStoreAppendResult SolidSyslogRecordStore_Append(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    const void* data,
    size_t dataSize
)
{
    enum SolidSyslogRecordStoreAppendResult result = SOLIDSYSLOG_RECORD_STORE_APPEND_SEAL_FAILED;
    if (RecordStore_AssembleRecord(recordStore, data, dataSize))
    {
        bool appended = SolidSyslogBlockDevice_Append(
            blockDevice,
            blockIndex,
            recordStore->Buffer,
            SolidSyslogRecordStore_RecordSize(recordStore, (uint16_t) dataSize)
        );
        result = appended ? SOLIDSYSLOG_RECORD_STORE_APPEND_SUCCEEDED : SOLIDSYSLOG_RECORD_STORE_APPEND_DEVICE_FAILED;
    }
    return result;
}

static inline bool RecordStore_AssembleRecord(struct SolidSyslogRecordStore* recordStore, const void* data, size_t size)
{
    RecordStore_MagicAddress(recordStore)[0] = MAGIC_BYTE_0;
    RecordStore_MagicAddress(recordStore)[1] = MAGIC_BYTE_1;

    /* Length is packed little-endian into the byte buffer - the on-disk
     * format is LE regardless of host (every supported target is LE; an
     * explicit pack keeps that invariant readable and side-steps MISRA
     * 21.15 which forbids memcpy between incompatible essential types). */
    uint16_t length = (uint16_t) size;
    uint8_t* lengthBytes = RecordStore_LengthAddress(recordStore);
    lengthBytes[0] = (uint8_t) (length & 0xFFU);
    lengthBytes[1] = (uint8_t) ((length >> 8) & 0xFFU);
    (void) memcpy(RecordStore_MessageAddress(recordStore), data, size);

    struct SolidSyslogSecurityRecord record = {
        RecordStore_ContentAddress(recordStore),
        RecordStore_ContentSize(size),
        CONTENT_HEADER_SIZE,
        RecordStore_TrailerAddress(recordStore, size)
    };
    bool sealed = recordStore->SecurityPolicy->SealRecord(recordStore->SecurityPolicy, &record);

    *RecordStore_SentFlagAddress(recordStore, size) = SENT_FLAG_UNSENT;
    return sealed;
}

static enum SolidSyslogRecordStoreReadResult RecordStore_ReadAndValidateRecord(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    size_t dataSize,
    uint16_t* length
);
static inline void RecordStore_CopyRecordData(
    struct SolidSyslogRecordStore* recordStore,
    uint16_t length,
    void* dst,
    size_t maxSize,
    size_t* bytesRead
);
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- blockIndex / offset are positional fields of the just-read record; distinct semantics
static inline void RecordStore_RememberCurrentRecord(
    struct SolidSyslogRecordStore* recordStore,
    size_t blockIndex,
    size_t offset,
    uint16_t length
);

// NOLINTEND(bugprone-easily-swappable-parameters)

enum SolidSyslogRecordStoreReadResult SolidSyslogRecordStore_Read(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    void* dst,
    size_t maxSize,
    size_t* bytesRead
)
{
    uint16_t length = 0U;
    size_t dataSize = SolidSyslogBlockDevice_Size(blockDevice, blockIndex);

    *bytesRead = 0;

    enum SolidSyslogRecordStoreReadResult result =
        RecordStore_ReadAndValidateRecord(recordStore, blockDevice, blockIndex, offset, dataSize, &length);

    if (result == SOLIDSYSLOG_RECORD_STORE_READ_SUCCEEDED)
    {
        RecordStore_CopyRecordData(recordStore, length, dst, maxSize, bytesRead);
        RecordStore_RememberCurrentRecord(recordStore, blockIndex, offset, length);
    }

    return result;
}

static inline enum SolidSyslogRecordStoreReadResult RecordStore_ReadSpan(
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    void* buf,
    size_t count,
    size_t dataSize
);
static inline bool RecordStore_ValidateHeader(struct SolidSyslogRecordStore* recordStore, uint16_t* length);
static inline bool RecordStore_OpenRecord(struct SolidSyslogRecordStore* recordStore, uint16_t length);

/* Each span is read only while it lies within the block's data, so a record cut
 * short - what a power cut mid-append leaves - is no record rather than a
 * device failure. */
static enum SolidSyslogRecordStoreReadResult RecordStore_ReadAndValidateRecord(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    size_t dataSize,
    uint16_t* length
)
{
    enum SolidSyslogRecordStoreReadResult result = RecordStore_ReadSpan(
        blockDevice,
        blockIndex,
        offset,
        RecordStore_ContentAddress(recordStore),
        CONTENT_HEADER_SIZE,
        dataSize
    );

    if (result == SOLIDSYSLOG_RECORD_STORE_READ_SUCCEEDED)
    {
        result = RecordStore_ValidateHeader(recordStore, length) ? SOLIDSYSLOG_RECORD_STORE_READ_SUCCEEDED
                                                                 : SOLIDSYSLOG_RECORD_STORE_READ_NO_RECORD;
    }

    if (result == SOLIDSYSLOG_RECORD_STORE_READ_SUCCEEDED)
    {
        result = RecordStore_ReadSpan(
            blockDevice,
            blockIndex,
            offset + CONTENT_HEADER_SIZE,
            RecordStore_MessageAddress(recordStore),
            *length,
            dataSize
        );
    }

    if (result == SOLIDSYSLOG_RECORD_STORE_READ_SUCCEEDED)
    {
        result = RecordStore_ReadSpan(
            blockDevice,
            blockIndex,
            RecordStore_TrailerOffset(offset, *length),
            RecordStore_TrailerAddress(recordStore, *length),
            recordStore->SecurityPolicy->TrailerSize,
            dataSize
        );
    }

    if (result == SOLIDSYSLOG_RECORD_STORE_READ_SUCCEEDED)
    {
        result = RecordStore_OpenRecord(recordStore, *length) ? SOLIDSYSLOG_RECORD_STORE_READ_SUCCEEDED
                                                              : SOLIDSYSLOG_RECORD_STORE_READ_NO_RECORD;
    }

    return result;
}

static inline bool RecordStore_SpanIsWithinData(size_t offset, size_t count, size_t dataSize);

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- offset, count and dataSize are a position, a length and a bound; distinct semantics
static inline enum SolidSyslogRecordStoreReadResult RecordStore_ReadSpan(
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    void* buf,
    size_t count,
    size_t dataSize
)
{
    enum SolidSyslogRecordStoreReadResult result = SOLIDSYSLOG_RECORD_STORE_READ_NO_RECORD;

    if (RecordStore_SpanIsWithinData(offset, count, dataSize))
    {
        result = SolidSyslogBlockDevice_Read(blockDevice, blockIndex, offset, buf, count)
                     ? SOLIDSYSLOG_RECORD_STORE_READ_SUCCEEDED
                     : SOLIDSYSLOG_RECORD_STORE_READ_DEVICE_FAILED;
    }

    return result;
}

static inline bool RecordStore_SpanIsWithinData(size_t offset, size_t count, size_t dataSize)
{
    return (offset + count) <= dataSize;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static inline bool RecordStore_IsMagicValid(struct SolidSyslogRecordStore* recordStore)
{
    return (RecordStore_MagicAddress(recordStore)[0] == MAGIC_BYTE_0) &&
           (RecordStore_MagicAddress(recordStore)[1] == MAGIC_BYTE_1);
}

static inline uint16_t RecordStore_RecordLength(struct SolidSyslogRecordStore* recordStore)
{
    /* Little-endian unpack - see AssembleRecord for the format invariant. */
    const uint8_t* lengthBytes = RecordStore_LengthAddress(recordStore);
    return (uint16_t) (((uint16_t) lengthBytes[0]) | (((uint16_t) lengthBytes[1]) << 8));
}

static inline bool RecordStore_IsValidLength(uint16_t length)
{
    return length <= SOLIDSYSLOG_MAX_MESSAGE_SIZE;
}

static inline bool RecordStore_ValidateHeader(struct SolidSyslogRecordStore* recordStore, uint16_t* length)
{
    bool valid = RecordStore_IsMagicValid(recordStore);

    if (valid)
    {
        *length = RecordStore_RecordLength(recordStore);
        valid = RecordStore_IsValidLength(*length);
    }

    return valid;
}

static inline bool RecordStore_OpenRecord(struct SolidSyslogRecordStore* recordStore, uint16_t length)
{
    struct SolidSyslogSecurityRecord record = {
        RecordStore_ContentAddress(recordStore),
        RecordStore_ContentSize(length),
        CONTENT_HEADER_SIZE,
        RecordStore_TrailerAddress(recordStore, length)
    };
    return recordStore->SecurityPolicy->OpenRecord(recordStore->SecurityPolicy, &record);
}

static inline size_t RecordStore_BoundedSize(uint16_t length, size_t maxSize)
{
    return (length < maxSize) ? length : maxSize;
}

static inline void RecordStore_CopyRecordData(
    struct SolidSyslogRecordStore* recordStore,
    uint16_t length,
    void* dst,
    size_t maxSize,
    size_t* bytesRead
)
{
    size_t copySize = RecordStore_BoundedSize(length, maxSize);
    (void) memcpy(dst, RecordStore_MessageAddress(recordStore), copySize);
    *bytesRead = copySize;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- blockIndex / offset are positional fields of the just-read record; distinct semantics
static inline void RecordStore_RememberCurrentRecord(
    struct SolidSyslogRecordStore* recordStore,
    size_t blockIndex,
    size_t offset,
    uint16_t length
)
{
    recordStore->LastReadBlockIndex = blockIndex;
    recordStore->LastSentFlagOffset = RecordStore_SentFlagOffset(recordStore, offset, length);
    recordStore->HasReadRecord = true;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static inline bool RecordStore_WriteSentFlag(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice
);

enum SolidSyslogRecordStoreMarkResult SolidSyslogRecordStore_MarkLastReadAsSent(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t* nextCursor
)
{
    enum SolidSyslogRecordStoreMarkResult result = SOLIDSYSLOG_RECORD_STORE_MARK_NOTHING_READ;

    if (recordStore->HasReadRecord)
    {
        result = SOLIDSYSLOG_RECORD_STORE_MARK_DEVICE_FAILED;

        if (RecordStore_WriteSentFlag(recordStore, blockDevice))
        {
            *nextCursor = recordStore->LastSentFlagOffset + SENT_FLAG_SIZE;
            recordStore->HasReadRecord = false;
            result = SOLIDSYSLOG_RECORD_STORE_MARK_SUCCEEDED;
        }
    }

    return result;
}

static inline bool RecordStore_WriteSentFlag(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice
)
{
    uint8_t flag = SENT_FLAG_SENT;
    return SolidSyslogBlockDevice_WriteAt(
        blockDevice,
        recordStore->LastReadBlockIndex,
        recordStore->LastSentFlagOffset,
        &flag,
        SENT_FLAG_SIZE
    );
}

void SolidSyslogRecordStore_ForgetLastRead(struct SolidSyslogRecordStore* recordStore)
{
    recordStore->HasReadRecord = false;
}

static bool RecordStore_AdvancePastSentRecord(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t* cursor,
    size_t blockSize,
    enum SolidSyslogRecordStoreScanResult* scanned
);
static inline enum SolidSyslogRecordStoreReadResult RecordStore_IsRecordSent(
    const struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t recordStart,
    uint16_t length,
    size_t blockSize,
    bool* sent
);
static inline void RecordStore_SkipRecord(
    const struct SolidSyslogRecordStore* recordStore,
    size_t* cursor,
    uint16_t length
);

size_t SolidSyslogRecordStore_FindFirstUnsent(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t blockSize,
    enum SolidSyslogRecordStoreScanResult* scanned
)
{
    size_t cursor = 0U;
    bool scanning = true;
    *scanned = SOLIDSYSLOG_RECORD_STORE_SCAN_CLEAN;

    while (scanning && (cursor < blockSize))
    {
        scanning = RecordStore_AdvancePastSentRecord(recordStore, blockDevice, blockIndex, &cursor, blockSize, scanned);
    }

    return cursor;
}

static bool RecordStore_AdvancePastSentRecord(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t* cursor,
    size_t blockSize,
    enum SolidSyslogRecordStoreScanResult* scanned
)
{
    uint16_t length = 0U;
    bool sent = false;
    bool advanced = false;
    enum SolidSyslogRecordStoreReadResult result =
        RecordStore_ReadAndValidateRecord(recordStore, blockDevice, blockIndex, *cursor, blockSize, &length);

    if (result == SOLIDSYSLOG_RECORD_STORE_READ_SUCCEEDED)
    {
        result = RecordStore_IsRecordSent(recordStore, blockDevice, blockIndex, *cursor, length, blockSize, &sent);
    }

    if (result == SOLIDSYSLOG_RECORD_STORE_READ_DEVICE_FAILED)
    {
        *cursor = blockSize;
        *scanned = SOLIDSYSLOG_RECORD_STORE_SCAN_DEVICE_FAILED;
    }
    else if (sent)
    {
        RecordStore_SkipRecord(recordStore, cursor, length);
        advanced = true;
    }
    else if (result == SOLIDSYSLOG_RECORD_STORE_READ_NO_RECORD)
    {
        *cursor = blockSize;
        *scanned = SOLIDSYSLOG_RECORD_STORE_SCAN_CORRUPT;
    }
    else
    {
        /* The first unsent record: the scan stops on it. */
    }

    return advanced;
}

/* A sent flag past the end of the data - the last byte of a record a power cut
 * interrupted - is taken as sent, so the scan steps over the record. The single
 * record skipped surfaces downstream as a sequenceId gap (RFC 5424 section
 * 6.3.1), which the receiver can detect. */
static inline enum SolidSyslogRecordStoreReadResult RecordStore_IsRecordSent(
    const struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t recordStart,
    uint16_t length,
    size_t blockSize,
    bool* sent
)
{
    uint8_t flag = SENT_FLAG_SENT;
    enum SolidSyslogRecordStoreReadResult result = RecordStore_ReadSpan(
        blockDevice,
        blockIndex,
        RecordStore_SentFlagOffset(recordStore, recordStart, length),
        &flag,
        SENT_FLAG_SIZE,
        blockSize
    );
    *sent = flag == SENT_FLAG_SENT;
    return result;
}

static inline void RecordStore_SkipRecord(
    const struct SolidSyslogRecordStore* recordStore,
    size_t* cursor,
    uint16_t length
)
{
    *cursor += SolidSyslogRecordStore_RecordSize(recordStore, length);
}
