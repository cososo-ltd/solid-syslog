#include "RecordStore.h"

#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogTunables.h"

#include <string.h>

enum
{
    MAGIC_SIZE         = 2,
    MAGIC_BYTE_0       = 0xA5,
    MAGIC_BYTE_1       = 0x5A,
    RECORD_LENGTH_SIZE = 2,
    SENT_FLAG_SIZE     = 1,
    SENT_FLAG_UNSENT   = 0xFF,
    SENT_FLAG_SENT     = 0x00
};

/* Each record in the buffer is laid out as
 *   [ magic | length | message | integrity | sentFlag ]
 * Field-address helpers chain outward from the record's base so the
 * field offsets are stated in one place each. The same chaining shapes
 * the block-offset helpers below. */

static inline uint8_t* MagicAddress(struct RecordStore* recordStore)
{
    return recordStore->buffer;
}

static inline uint8_t* LengthAddress(struct RecordStore* recordStore)
{
    return MagicAddress(recordStore) + MAGIC_SIZE;
}

static inline uint8_t* MessageAddress(struct RecordStore* recordStore)
{
    return LengthAddress(recordStore) + RECORD_LENGTH_SIZE;
}

static inline uint8_t* IntegrityChecksumAddress(struct RecordStore* recordStore, size_t dataSize)
{
    return MessageAddress(recordStore) + dataSize;
}

static inline uint8_t* SentFlagAddress(struct RecordStore* recordStore, size_t dataSize)
{
    return IntegrityChecksumAddress(recordStore, dataSize) + recordStore->securityPolicy->integritySize;
}

static inline uint8_t* IntegrityRegionAddress(struct RecordStore* recordStore)
{
    return MagicAddress(recordStore);
}

static inline uint16_t IntegrityRegionSize(size_t dataSize)
{
    return (uint16_t) (MAGIC_SIZE + RECORD_LENGTH_SIZE + dataSize);
}

static inline size_t IntegrityChecksumOffset(size_t recordStart, uint16_t dataLength)
{
    return recordStart + MAGIC_SIZE + RECORD_LENGTH_SIZE + dataLength;
}

static inline size_t SentFlagOffset(const struct RecordStore* recordStore, size_t recordStart, uint16_t dataLength)
{
    return IntegrityChecksumOffset(recordStart, dataLength) + recordStore->securityPolicy->integritySize;
}

void RecordStore_Init(struct RecordStore* recordStore, struct SolidSyslogSecurityPolicy* securityPolicy)
{
    recordStore->securityPolicy     = securityPolicy;
    recordStore->hasReadRecord      = false;
    recordStore->lastReadBlockIndex = 0;
    recordStore->lastSentFlagOffset = 0;
}

size_t RecordStore_RecordSize(const struct RecordStore* recordStore, uint16_t dataLength)
{
    return (size_t) MAGIC_SIZE + RECORD_LENGTH_SIZE + dataLength + recordStore->securityPolicy->integritySize + SENT_FLAG_SIZE;
}

static inline void AssembleRecord(struct RecordStore* recordStore, const void* data, size_t size);

bool RecordStore_Append(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, const void* data, size_t dataSize)
{
    AssembleRecord(recordStore, data, dataSize);
    return SolidSyslogBlockDevice_Append(blockDevice, blockIndex, recordStore->buffer, RecordStore_RecordSize(recordStore, (uint16_t) dataSize));
}

static inline void AssembleRecord(struct RecordStore* recordStore, const void* data, size_t size)
{
    MagicAddress(recordStore)[0] = MAGIC_BYTE_0;
    MagicAddress(recordStore)[1] = MAGIC_BYTE_1;

    uint16_t length = (uint16_t) size;
    memcpy(LengthAddress(recordStore), &length, RECORD_LENGTH_SIZE);
    memcpy(MessageAddress(recordStore), data, size);

    recordStore->securityPolicy->ComputeIntegrity(IntegrityRegionAddress(recordStore), IntegrityRegionSize(size), IntegrityChecksumAddress(recordStore, size));

    *SentFlagAddress(recordStore, size) = SENT_FLAG_UNSENT;
}

static bool        ReadAndValidateRecord(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t offset,
                                         uint16_t* length);
static inline void CopyRecordData(struct RecordStore* recordStore, uint16_t length, void* dst, size_t maxSize, size_t* bytesRead);
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- blockIndex / offset are positional fields of the just-read record; distinct semantics
static inline void RememberCurrentRecord(struct RecordStore* recordStore, size_t blockIndex, size_t offset, uint16_t length);

bool RecordStore_Read(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t offset, void* dst, size_t maxSize,
                      size_t* bytesRead)
{
    uint16_t length = 0;
    bool     read   = false;

    *bytesRead = 0;

    if (ReadAndValidateRecord(recordStore, blockDevice, blockIndex, offset, &length))
    {
        CopyRecordData(recordStore, length, dst, maxSize, bytesRead);
        RememberCurrentRecord(recordStore, blockIndex, offset, length);
        read = *bytesRead > 0;
    }

    return read;
}

static inline bool ReadRecordHeader(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t offset);
static inline bool ValidateHeader(struct RecordStore* recordStore, uint16_t* length);
static inline bool ReadRecordBody(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t offset,
                                  uint16_t length);
static inline bool ReadIntegrityChecksum(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t recordStart,
                                         uint16_t dataLength);
static inline bool VerifyIntegrity(struct RecordStore* recordStore, uint16_t length);

static bool ReadAndValidateRecord(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t offset,
                                  uint16_t* length)
{
    return ReadRecordHeader(recordStore, blockDevice, blockIndex, offset) && ValidateHeader(recordStore, length) &&
           ReadRecordBody(recordStore, blockDevice, blockIndex, offset, *length) &&
           ReadIntegrityChecksum(recordStore, blockDevice, blockIndex, offset, *length) && VerifyIntegrity(recordStore, *length);
}

static inline bool ReadRecordHeader(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t offset)
{
    return SolidSyslogBlockDevice_Read(blockDevice, blockIndex, offset, IntegrityRegionAddress(recordStore), MAGIC_SIZE + RECORD_LENGTH_SIZE);
}

static inline bool IsMagicValid(struct RecordStore* recordStore)
{
    return (MagicAddress(recordStore)[0] == MAGIC_BYTE_0) && (MagicAddress(recordStore)[1] == MAGIC_BYTE_1);
}

static inline uint16_t RecordLength(struct RecordStore* recordStore)
{
    uint16_t length = 0;
    memcpy(&length, LengthAddress(recordStore), RECORD_LENGTH_SIZE);
    return length;
}

static inline bool IsValidLength(uint16_t length)
{
    return length <= SOLIDSYSLOG_MAX_MESSAGE_SIZE;
}

static inline bool ValidateHeader(struct RecordStore* recordStore, uint16_t* length)
{
    bool valid = IsMagicValid(recordStore);

    if (valid)
    {
        *length = RecordLength(recordStore);
        valid   = IsValidLength(*length);
    }

    return valid;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- offset is a block position, length is a data size; distinct semantics
static inline bool ReadRecordBody(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t offset,
                                  uint16_t length)
{
    return SolidSyslogBlockDevice_Read(blockDevice, blockIndex, offset + MAGIC_SIZE + RECORD_LENGTH_SIZE, MessageAddress(recordStore), length);
}

static inline bool ReadIntegrityChecksum(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t recordStart,
                                         uint16_t dataLength)
{
    return SolidSyslogBlockDevice_Read(blockDevice, blockIndex, IntegrityChecksumOffset(recordStart, dataLength),
                                       IntegrityChecksumAddress(recordStore, dataLength), recordStore->securityPolicy->integritySize);
}

static inline bool VerifyIntegrity(struct RecordStore* recordStore, uint16_t length)
{
    return recordStore->securityPolicy->VerifyIntegrity(IntegrityRegionAddress(recordStore), IntegrityRegionSize(length),
                                                        IntegrityChecksumAddress(recordStore, length));
}

static inline size_t BoundedSize(uint16_t length, size_t maxSize)
{
    return (length < maxSize) ? length : maxSize;
}

static inline void CopyRecordData(struct RecordStore* recordStore, uint16_t length, void* dst, size_t maxSize, size_t* bytesRead)
{
    size_t copySize = BoundedSize(length, maxSize);
    memcpy(dst, MessageAddress(recordStore), copySize);
    *bytesRead = copySize;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- blockIndex / offset are positional fields of the just-read record; distinct semantics
static inline void RememberCurrentRecord(struct RecordStore* recordStore, size_t blockIndex, size_t offset, uint16_t length)
{
    recordStore->lastReadBlockIndex = blockIndex;
    recordStore->lastSentFlagOffset = SentFlagOffset(recordStore, offset, length);
    recordStore->hasReadRecord      = true;
}

static inline bool WriteSentFlag(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice);

bool RecordStore_MarkLastReadAsSent(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t* nextCursor)
{
    bool marked = false;

    if (recordStore->hasReadRecord && WriteSentFlag(recordStore, blockDevice))
    {
        *nextCursor                = recordStore->lastSentFlagOffset + SENT_FLAG_SIZE;
        recordStore->hasReadRecord = false;
        marked                     = true;
    }

    return marked;
}

static inline bool WriteSentFlag(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice)
{
    uint8_t flag = SENT_FLAG_SENT;
    return SolidSyslogBlockDevice_WriteAt(blockDevice, recordStore->lastReadBlockIndex, recordStore->lastSentFlagOffset, &flag, SENT_FLAG_SIZE);
}

void RecordStore_ForgetLastRead(struct RecordStore* recordStore)
{
    recordStore->hasReadRecord = false;
}

static bool        AdvancePastSentRecord(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t* cursor,
                                         size_t blockSize, bool* corrupt);
static inline bool IsRecordSent(const struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t recordStart,
                                uint16_t length);
static inline void SkipRecord(const struct RecordStore* recordStore, size_t* cursor, uint16_t length);

size_t RecordStore_FindFirstUnsent(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t blockSize,
                                   bool* corrupt)
{
    size_t cursor   = 0;
    bool   scanning = true;
    *corrupt        = false;

    while (scanning && (cursor < blockSize))
    {
        scanning = AdvancePastSentRecord(recordStore, blockDevice, blockIndex, &cursor, blockSize, corrupt);
    }

    return cursor;
}

static bool AdvancePastSentRecord(struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t* cursor,
                                  size_t blockSize, bool* corrupt)
{
    uint16_t length   = 0;
    bool     advanced = false;

    if (ReadAndValidateRecord(recordStore, blockDevice, blockIndex, *cursor, &length))
    {
        if (IsRecordSent(recordStore, blockDevice, blockIndex, *cursor, length))
        {
            SkipRecord(recordStore, cursor, length);
            advanced = true;
        }
    }
    else
    {
        *cursor  = blockSize;
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
static inline bool IsRecordSent(const struct RecordStore* recordStore, struct SolidSyslogBlockDevice* blockDevice, size_t blockIndex, size_t recordStart,
                                uint16_t length)
{
    uint8_t flag = SENT_FLAG_SENT;
    SolidSyslogBlockDevice_Read(blockDevice, blockIndex, SentFlagOffset(recordStore, recordStart, length), &flag, SENT_FLAG_SIZE);
    return flag == SENT_FLAG_SENT;
}

static inline void SkipRecord(const struct RecordStore* recordStore, size_t* cursor, uint16_t length)
{
    *cursor += RecordStore_RecordSize(recordStore, length);
}
