#ifndef SOLIDSYSLOG_RECORDSTORE_H
#define SOLIDSYSLOG_RECORDSTORE_H

#include "SolidSyslog.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct SolidSyslogFile;

enum
{
    RECORD_BUFFER_SIZE = 2 + 2 + SOLIDSYSLOG_MAX_MESSAGE_SIZE + SOLIDSYSLOG_MAX_INTEGRITY_SIZE + 1
};

struct RecordStore
{
    struct SolidSyslogSecurityPolicy* securityPolicy;
    bool                              hasReadRecord;
    size_t                            lastSentFlagFileOffset;
    uint8_t                           buffer[RECORD_BUFFER_SIZE];
};

void RecordStore_Init(struct RecordStore* recordStore, struct SolidSyslogSecurityPolicy* securityPolicy);

size_t RecordStore_RecordSize(const struct RecordStore* recordStore, uint16_t dataLength);

bool RecordStore_Append(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t position, const void* data, size_t dataSize);

bool RecordStore_Read(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t offset, void* dst, size_t maxSize, size_t* bytesRead);

bool RecordStore_MarkLastReadAsSent(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t* nextCursor);

void RecordStore_ForgetLastRead(struct RecordStore* recordStore);

size_t RecordStore_FindFirstUnsent(struct RecordStore* recordStore, struct SolidSyslogFile* file, size_t fileSize, bool* corrupt);

#endif /* SOLIDSYSLOG_RECORDSTORE_H */
