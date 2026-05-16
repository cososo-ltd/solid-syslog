#ifndef SOLIDSYSLOG_RECORDSTORE_H
#define SOLIDSYSLOG_RECORDSTORE_H

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

struct RecordStore
{
    struct SolidSyslogSecurityPolicy* SecurityPolicy;
    bool HasReadRecord;
    size_t LastReadBlockIndex;
    size_t LastSentFlagOffset;
    uint8_t Buffer[RECORD_BUFFER_SIZE];
};

void RecordStore_Init(struct RecordStore* recordStore, struct SolidSyslogSecurityPolicy* securityPolicy);

size_t RecordStore_RecordSize(const struct RecordStore* recordStore, uint16_t dataLength);

bool RecordStore_Append(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    const void* data,
    size_t dataSize
);

bool RecordStore_Read(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t offset,
    void* dst,
    size_t maxSize,
    size_t* bytesRead
);

bool RecordStore_MarkLastReadAsSent(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t* nextCursor
);

void RecordStore_ForgetLastRead(struct RecordStore* recordStore);

size_t RecordStore_FindFirstUnsent(
    struct RecordStore* recordStore,
    struct SolidSyslogBlockDevice* blockDevice,
    size_t blockIndex,
    size_t blockSize,
    bool* corrupt
);

#endif /* SOLIDSYSLOG_RECORDSTORE_H */
