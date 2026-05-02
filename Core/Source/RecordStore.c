#include "RecordStore.h"

#include "SolidSyslogFile.h"

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
 * the file-offset helpers below. */

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

static inline size_t IntegrityChecksumFileOffset(size_t recordStart, uint16_t dataLength)
{
    return recordStart + MAGIC_SIZE + RECORD_LENGTH_SIZE + dataLength;
}

static inline size_t SentFlagFileOffset(const struct RecordStore* recordStore, size_t recordStart, uint16_t dataLength)
{
    return IntegrityChecksumFileOffset(recordStart, dataLength) + recordStore->securityPolicy->integritySize;
}

void RecordStore_Init(struct RecordStore* recordStore, struct SolidSyslogSecurityPolicy* securityPolicy)
{
    recordStore->securityPolicy         = securityPolicy;
    recordStore->hasReadRecord          = false;
    recordStore->lastSentFlagFileOffset = 0;
}

size_t RecordStore_RecordSize(const struct RecordStore* recordStore, uint16_t dataLength)
{
    return (size_t) MAGIC_SIZE + RECORD_LENGTH_SIZE + dataLength + recordStore->securityPolicy->integritySize + SENT_FLAG_SIZE;
}

static inline void AssembleRecord(struct RecordStore* recordStore, const void* data, size_t size);

bool RecordStore_Append(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t position, const void* data, size_t dataSize)
{
    AssembleRecord(recordStore, data, dataSize);

    SolidSyslogFile_SeekTo(file, position);

    return SolidSyslogFile_Write(file, recordStore->buffer, RecordStore_RecordSize(recordStore, (uint16_t) dataSize));
}

static inline void AssembleRecord(struct RecordStore* recordStore, const void* data, size_t size)
{
    MagicAddress(recordStore)[0] = MAGIC_BYTE_0;
    MagicAddress(recordStore)[1] = MAGIC_BYTE_1;

    uint16_t length = (uint16_t) size;
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) -- memcpy with bounded size
    memcpy(LengthAddress(recordStore), &length, RECORD_LENGTH_SIZE);
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) -- memcpy with bounded size
    memcpy(MessageAddress(recordStore), data, size);

    recordStore->securityPolicy->ComputeIntegrity(IntegrityRegionAddress(recordStore), IntegrityRegionSize(size), IntegrityChecksumAddress(recordStore, size));

    *SentFlagAddress(recordStore, size) = SENT_FLAG_UNSENT;
}

static bool        ReadAndValidateRecord(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t offset, uint16_t* length);
static inline void CopyRecordData(struct RecordStore* recordStore, uint16_t length, void* dst, size_t maxSize, size_t* bytesRead);
static inline void RememberCurrentRecord(struct RecordStore* recordStore, size_t offset, uint16_t length);

bool RecordStore_Read(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t offset, void* dst, size_t maxSize, size_t* bytesRead)
{
    uint16_t length = 0;
    bool     read   = false;

    *bytesRead = 0;

    if (ReadAndValidateRecord(recordStore, file, offset, &length))
    {
        CopyRecordData(recordStore, length, dst, maxSize, bytesRead);
        RememberCurrentRecord(recordStore, offset, length);
        read = *bytesRead > 0;
    }

    return read;
}

static inline bool ReadRecordHeader(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t offset);
static inline bool ValidateHeader(struct RecordStore* recordStore, uint16_t* length);
static inline bool ReadRecordBody(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t offset, uint16_t length);
static inline bool ReadIntegrityChecksum(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t recordStart, uint16_t dataLength);
static inline bool VerifyIntegrity(struct RecordStore* recordStore, uint16_t length);

static bool ReadAndValidateRecord(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t offset, uint16_t* length)
{
    return ReadRecordHeader(recordStore, file, offset) && ValidateHeader(recordStore, length) && ReadRecordBody(recordStore, file, offset, *length) &&
           ReadIntegrityChecksum(recordStore, file, offset, *length) && VerifyIntegrity(recordStore, *length);
}

static inline bool ReadRecordHeader(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t offset)
{
    SolidSyslogFile_SeekTo(file, offset);
    return SolidSyslogFile_Read(file, IntegrityRegionAddress(recordStore), MAGIC_SIZE + RECORD_LENGTH_SIZE);
}

static inline bool IsMagicValid(struct RecordStore* recordStore)
{
    return (MagicAddress(recordStore)[0] == MAGIC_BYTE_0) && (MagicAddress(recordStore)[1] == MAGIC_BYTE_1);
}

static inline uint16_t RecordLength(struct RecordStore* recordStore)
{
    uint16_t length = 0;
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) -- memcpy with bounded size
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

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- offset is a file position, length is a data size; distinct semantics
static inline bool ReadRecordBody(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t offset, uint16_t length)
{
    SolidSyslogFile_SeekTo(file, offset + MAGIC_SIZE + RECORD_LENGTH_SIZE);
    return SolidSyslogFile_Read(file, MessageAddress(recordStore), length);
}

static inline bool ReadIntegrityChecksum(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t recordStart, uint16_t dataLength)
{
    SolidSyslogFile_SeekTo(file, IntegrityChecksumFileOffset(recordStart, dataLength));
    return SolidSyslogFile_Read(file, IntegrityChecksumAddress(recordStore, dataLength), recordStore->securityPolicy->integritySize);
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
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) -- memcpy with bounded size
    memcpy(dst, MessageAddress(recordStore), copySize);
    *bytesRead = copySize;
}

static inline void RememberCurrentRecord(struct RecordStore* recordStore, size_t offset, uint16_t length)
{
    recordStore->lastSentFlagFileOffset = SentFlagFileOffset(recordStore, offset, length);
    recordStore->hasReadRecord          = true;
}

static inline bool WriteSentFlag(struct RecordStore* recordStore, struct SolidSyslogFile* file);

bool RecordStore_MarkLastReadAsSent(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t* nextCursor)
{
    bool marked = false;

    if (recordStore->hasReadRecord && WriteSentFlag(recordStore, file))
    {
        *nextCursor                = recordStore->lastSentFlagFileOffset + SENT_FLAG_SIZE;
        recordStore->hasReadRecord = false;
        marked                     = true;
    }

    return marked;
}

static inline bool WriteSentFlag(struct RecordStore* recordStore, struct SolidSyslogFile* file)
{
    uint8_t flag = SENT_FLAG_SENT;

    SolidSyslogFile_SeekTo(file, recordStore->lastSentFlagFileOffset);
    return SolidSyslogFile_Write(file, &flag, SENT_FLAG_SIZE);
}

void RecordStore_ForgetLastRead(struct RecordStore* recordStore)
{
    recordStore->hasReadRecord = false;
}

static bool        AdvancePastSentRecord(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t* cursor, size_t fileSize, bool* corrupt);
static inline bool ReadSentFlag(struct SolidSyslogFile* file, size_t sentFlagOffset, uint8_t* flag);
static inline bool IsRecordSent(const struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t recordStart, uint16_t length);
static inline void SkipRecord(const struct RecordStore* recordStore, size_t* cursor, uint16_t length);

size_t RecordStore_FindFirstUnsent(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t fileSize, bool* corrupt)
{
    size_t cursor   = 0;
    bool   scanning = true;
    *corrupt        = false;

    while (scanning && (cursor < fileSize))
    {
        scanning = AdvancePastSentRecord(recordStore, file, &cursor, fileSize, corrupt);
    }

    return cursor;
}

static bool AdvancePastSentRecord(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t* cursor, size_t fileSize, bool* corrupt)
{
    uint16_t length   = 0;
    bool     advanced = false;

    if (ReadAndValidateRecord(recordStore, file, *cursor, &length))
    {
        if (IsRecordSent(recordStore, file, *cursor, length))
        {
            SkipRecord(recordStore, cursor, length);
            advanced = true;
        }
    }
    else
    {
        *cursor  = fileSize;
        *corrupt = true;
    }

    return advanced;
}

static inline bool ReadSentFlag(struct SolidSyslogFile* file, size_t sentFlagOffset, uint8_t* flag)
{
    SolidSyslogFile_SeekTo(file, sentFlagOffset);
    return SolidSyslogFile_Read(file, flag, SENT_FLAG_SIZE);
}

static inline bool IsRecordSent(const struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t recordStart, uint16_t length)
{
    uint8_t flag = SENT_FLAG_SENT;
    ReadSentFlag(file, SentFlagFileOffset(recordStore, recordStart, length), &flag);
    return flag == SENT_FLAG_SENT;
}

static inline void SkipRecord(const struct RecordStore* recordStore, size_t* cursor, uint16_t length)
{
    *cursor += RecordStore_RecordSize(recordStore, length);
}
