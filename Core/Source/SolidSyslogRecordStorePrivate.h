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

bool SolidSyslogRecordStore_Append(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    const void* data,
    size_t dataSize
);

bool SolidSyslogRecordStore_Read(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    void* dst,
    size_t maxSize,
    size_t* bytesRead
);

bool SolidSyslogRecordStore_MarkLastReadAsSent(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t* nextCursor
);

void SolidSyslogRecordStore_ForgetLastRead(struct SolidSyslogRecordStore* recordStore);

size_t SolidSyslogRecordStore_FindFirstUnsent(
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t blockSize,
    bool* corrupt
);

#endif /* SOLIDSYSLOGRECORDSTOREPRIVATE_H */
