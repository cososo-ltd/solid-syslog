#include "RecordStore.h"

#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogTunables.h"

#include <string.h>

enum
{
    MAGIC_SIZE = 2,
    MAGIC_BYTE_0 = 0xA5,
    MAGIC_BYTE_1 = 0x5A,
    RECORD_LENGTH_SIZE = 2,
    SENT_FLAG_SIZE = 1,
    SENT_FLAG_UNSENT = 0xFF,
    SENT_FLAG_SENT = 0x00
};

/* Each record in the buffer is laid out as
 *   [ magic | length | message | integrity | sentFlag ]
 * Field-address helpers chain outward from the record's base so the
 * field offsets are stated in one place each. The same chaining shapes
 * the block-offset helpers below. */

static inline uint8_t* RecordStore_MagicAddress(struct RecordStore* recordStore)
{
    return recordStore->Buffer;
}

static inline uint8_t* RecordStore_LengthAddress(struct RecordStore* recordStore)
{
    return RecordStore_MagicAddress(recordStore) + MAGIC_SIZE;
}

static inline uint8_t* RecordStore_MessageAddress(struct RecordStore* recordStore)
{
    return RecordStore_LengthAddress(recordStore) + RECORD_LENGTH_SIZE;
}

static inline uint8_t* RecordStore_IntegrityChecksumAddress(struct RecordStore* recordStore, size_t dataSize)
{
    return RecordStore_MessageAddress(recordStore) + dataSize;
}

static inline uint8_t* RecordStore_SentFlagAddress(struct RecordStore* recordStore, size_t dataSize)
{
    return RecordStore_IntegrityChecksumAddress(recordStore, dataSize) + recordStore->SecurityPolicy->IntegritySize;
}

static inline uint8_t* RecordStore_IntegrityRegionAddress(struct RecordStore* recordStore)
{
    return RecordStore_MagicAddress(recordStore);
}

static inline uint16_t RecordStore_IntegrityRegionSize(size_t dataSize)
{
    return (uint16_t) (MAGIC_SIZE + RECORD_LENGTH_SIZE + dataSize);
}

static inline size_t RecordStore_IntegrityChecksumOffset(size_t recordStart, uint16_t dataLength)
{
    return recordStart + MAGIC_SIZE + RECORD_LENGTH_SIZE + dataLength;
}

static inline size_t RecordStore_SentFlagOffset(
    const struct RecordStore* recordStore,
    size_t recordStart,
    uint16_t dataLength
)
{
    return RecordStore_IntegrityChecksumOffset(recordStart, dataLength) + recordStore->SecurityPolicy->IntegritySize;
}

void RecordStore_Init(struct RecordStore* recordStore, struct SolidSyslogSecurityPolicy* securityPolicy)
{
    recordStore->SecurityPolicy = securityPolicy;
    recordStore->HasReadRecord = false;
    recordStore->LastReadBlockIndex = 0;
    recordStore->LastSentFlagOffset = 0;
}

size_t RecordStore_RecordSize(const struct RecordStore* recordStore, uint16_t dataLength)
{
    return (size_t) MAGIC_SIZE + RECORD_LENGTH_SIZE + dataLength + recordStore->SecurityPolicy->IntegritySize +
           SENT_FLAG_SIZE;
}

static inline void RecordStore_AssembleRecord(struct RecordStore* recordStore, const void* data, size_t size);

bool RecordStore_Append(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    const void* data,
    size_t dataSize
)
{
    RecordStore_AssembleRecord(recordStore, data, dataSize);
    return SolidSyslogBlockDevice_Append(
        blockDevice,
        blockIndex,
        recordStore->Buffer,
        RecordStore_RecordSize(recordStore, (uint16_t) dataSize)
    );
}

static inline void RecordStore_AssembleRecord(struct RecordStore* recordStore, const void* data, size_t size)
{
    RecordStore_MagicAddress(recordStore)[0] = MAGIC_BYTE_0;
    RecordStore_MagicAddress(recordStore)[1] = MAGIC_BYTE_1;

    uint16_t length = (uint16_t) size;
    memcpy(RecordStore_LengthAddress(recordStore), &length, RECORD_LENGTH_SIZE);
    memcpy(RecordStore_MessageAddress(recordStore), data, size);

    recordStore->SecurityPolicy->ComputeIntegrity(
        RecordStore_IntegrityRegionAddress(recordStore),
        RecordStore_IntegrityRegionSize(size),
        RecordStore_IntegrityChecksumAddress(recordStore, size)
    );

    *RecordStore_SentFlagAddress(recordStore, size) = SENT_FLAG_UNSENT;
}

static bool RecordStore_ReadAndValidateRecord(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    uint16_t* length
);
static inline void RecordStore_CopyRecordData(
    struct RecordStore* recordStore,
    uint16_t length,
    void* dst,
    size_t maxSize,
    size_t* bytesRead
);
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- blockIndex / offset are positional fields of the just-read record; distinct semantics
static inline void RecordStore_RememberCurrentRecord(
    struct RecordStore* recordStore,
    size_t blockIndex,
    size_t offset,
    uint16_t length
);

// NOLINTEND(bugprone-easily-swappable-parameters)

bool RecordStore_Read(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    void* dst,
    size_t maxSize,
    size_t* bytesRead
)
{
    uint16_t length = 0;
    bool read = false;

    *bytesRead = 0;

    if (RecordStore_ReadAndValidateRecord(recordStore, blockDevice, blockIndex, offset, &length))
    {
        RecordStore_CopyRecordData(recordStore, length, dst, maxSize, bytesRead);
        RecordStore_RememberCurrentRecord(recordStore, blockIndex, offset, length);
        read = *bytesRead > 0;
    }

    return read;
}

static inline bool RecordStore_ReadRecordHeader(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset
);
static inline bool RecordStore_ValidateHeader(struct RecordStore* recordStore, uint16_t* length);
static inline bool RecordStore_ReadRecordBody(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    uint16_t length
);
static inline bool RecordStore_ReadIntegrityChecksum(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t recordStart,
    uint16_t dataLength
);
static inline bool RecordStore_VerifyIntegrity(struct RecordStore* recordStore, uint16_t length);

static bool RecordStore_ReadAndValidateRecord(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    uint16_t* length
)
{
    return RecordStore_ReadRecordHeader(recordStore, blockDevice, blockIndex, offset) &&
           RecordStore_ValidateHeader(recordStore, length) &&
           RecordStore_ReadRecordBody(recordStore, blockDevice, blockIndex, offset, *length) &&
           RecordStore_ReadIntegrityChecksum(recordStore, blockDevice, blockIndex, offset, *length) &&
           RecordStore_VerifyIntegrity(recordStore, *length);
}

static inline bool RecordStore_ReadRecordHeader(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset
)
{
    return SolidSyslogBlockDevice_Read(
        blockDevice,
        blockIndex,
        offset,
        RecordStore_IntegrityRegionAddress(recordStore),
        MAGIC_SIZE + RECORD_LENGTH_SIZE
    );
}

static inline bool RecordStore_IsMagicValid(struct RecordStore* recordStore)
{
    return (RecordStore_MagicAddress(recordStore)[0] == MAGIC_BYTE_0) &&
           (RecordStore_MagicAddress(recordStore)[1] == MAGIC_BYTE_1);
}

static inline uint16_t RecordStore_RecordLength(struct RecordStore* recordStore)
{
    uint16_t length = 0;
    memcpy(&length, RecordStore_LengthAddress(recordStore), RECORD_LENGTH_SIZE);
    return length;
}

static inline bool RecordStore_IsValidLength(uint16_t length)
{
    return length <= SOLIDSYSLOG_MAX_MESSAGE_SIZE;
}

static inline bool RecordStore_ValidateHeader(struct RecordStore* recordStore, uint16_t* length)
{
    bool valid = RecordStore_IsMagicValid(recordStore);

    if (valid)
    {
        *length = RecordStore_RecordLength(recordStore);
        valid = RecordStore_IsValidLength(*length);
    }

    return valid;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- offset is a block position, length is a data size; distinct semantics
static inline bool RecordStore_ReadRecordBody(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    uint16_t length
)
{
    return SolidSyslogBlockDevice_Read(
        blockDevice,
        blockIndex,
        offset + MAGIC_SIZE + RECORD_LENGTH_SIZE,
        RecordStore_MessageAddress(recordStore),
        length
    );
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static inline bool RecordStore_ReadIntegrityChecksum(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t recordStart,
    uint16_t dataLength
)
{
    return SolidSyslogBlockDevice_Read(
        blockDevice,
        blockIndex,
        RecordStore_IntegrityChecksumOffset(recordStart, dataLength),
        RecordStore_IntegrityChecksumAddress(recordStore, dataLength),
        recordStore->SecurityPolicy->IntegritySize
    );
}

static inline bool RecordStore_VerifyIntegrity(struct RecordStore* recordStore, uint16_t length)
{
    return recordStore->SecurityPolicy->VerifyIntegrity(
        RecordStore_IntegrityRegionAddress(recordStore),
        RecordStore_IntegrityRegionSize(length),
        RecordStore_IntegrityChecksumAddress(recordStore, length)
    );
}

static inline size_t RecordStore_BoundedSize(uint16_t length, size_t maxSize)
{
    return (length < maxSize) ? length : maxSize;
}

static inline void RecordStore_CopyRecordData(
    struct RecordStore* recordStore,
    uint16_t length,
    void* dst,
    size_t maxSize,
    size_t* bytesRead
)
{
    size_t copySize = RecordStore_BoundedSize(length, maxSize);
    memcpy(dst, RecordStore_MessageAddress(recordStore), copySize);
    *bytesRead = copySize;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- blockIndex / offset are positional fields of the just-read record; distinct semantics
static inline void RecordStore_RememberCurrentRecord(
    struct RecordStore* recordStore,
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
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice
);

bool RecordStore_MarkLastReadAsSent(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t* nextCursor
)
{
    bool marked = false;

    if (recordStore->HasReadRecord && RecordStore_WriteSentFlag(recordStore, blockDevice))
    {
        *nextCursor = recordStore->LastSentFlagOffset + SENT_FLAG_SIZE;
        recordStore->HasReadRecord = false;
        marked = true;
    }

    return marked;
}

static inline bool RecordStore_WriteSentFlag(
    struct RecordStore* recordStore,
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

void RecordStore_ForgetLastRead(struct RecordStore* recordStore)
{
    recordStore->HasReadRecord = false;
}

static bool RecordStore_AdvancePastSentRecord(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t* cursor,
    size_t blockSize,
    bool* corrupt
);
static inline bool RecordStore_IsRecordSent(
    const struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t recordStart,
    uint16_t length
);
static inline void RecordStore_SkipRecord(const struct RecordStore* recordStore, size_t* cursor, uint16_t length);

size_t RecordStore_FindFirstUnsent(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t blockSize,
    bool* corrupt
)
{
    size_t cursor = 0;
    bool scanning = true;
    *corrupt = false;

    while (scanning && (cursor < blockSize))
    {
        scanning = RecordStore_AdvancePastSentRecord(recordStore, blockDevice, blockIndex, &cursor, blockSize, corrupt);
    }

    return cursor;
}

static bool RecordStore_AdvancePastSentRecord(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t* cursor,
    size_t blockSize,
    bool* corrupt
)
{
    uint16_t length = 0;
    bool advanced = false;

    if (RecordStore_ReadAndValidateRecord(recordStore, blockDevice, blockIndex, *cursor, &length))
    {
        if (RecordStore_IsRecordSent(recordStore, blockDevice, blockIndex, *cursor, length))
        {
            RecordStore_SkipRecord(recordStore, cursor, length);
            advanced = true;
        }
    }
    else
    {
        *cursor = blockSize;
        *corrupt = true;
    }

    return advanced;
}

/* `flag` defaults to SENT_FLAG_SENT and the Read result is intentionally
 * ignored: a sent-flag we cannot read is treated as already-sent so the
 * scan keeps walking the block. The alternative — stop scanning, refuse
 * to advance — would jam the logger on one bad byte and skip every record
 * that follows in the same block. The single skipped record surfaces
 * downstream as a sequenceId gap (RFC 5424 §6.3.1), which the receiver
 * can detect; an integrator-supplied error reporter will surface
 * persistent media errors directly when that path lands. */
static inline bool RecordStore_IsRecordSent(
    const struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t recordStart,
    uint16_t length
)
{
    uint8_t flag = SENT_FLAG_SENT;
    SolidSyslogBlockDevice_Read(
        blockDevice,
        blockIndex,
        RecordStore_SentFlagOffset(recordStore, recordStart, length),
        &flag,
        SENT_FLAG_SIZE
    );
    return flag == SENT_FLAG_SENT;
}

static inline void RecordStore_SkipRecord(const struct RecordStore* recordStore, size_t* cursor, uint16_t length)
{
    *cursor += RecordStore_RecordSize(recordStore, length);
}
