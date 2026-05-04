#include "SolidSyslog.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "SolidSyslogBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogStructuredData.h"
#include "SolidSyslogStringFunction.h"
#include "SolidSyslogTimestamp.h"

struct SolidSyslogFormatter;
struct SolidSyslogStructuredData;

enum
{
    SOLIDSYSLOG_MAX_APP_NAME_SIZE   = 49,
    SOLIDSYSLOG_MAX_HOSTNAME_SIZE   = 256,
    SOLIDSYSLOG_MAX_MSGID_SIZE      = 33,
    SOLIDSYSLOG_MAX_PROCESS_ID_SIZE = 129
};

static inline int16_t     AbsoluteInt16(int16_t value);
static inline bool        CaptureTimestamp(struct SolidSyslogTimestamp* ts, SolidSyslogClockFunction clock);
static inline uint8_t     CombineFacilityAndSeverity(uint8_t facility, uint8_t severity);
static inline bool        FacilityIsValid(uint8_t facility);
static inline bool        FetchFromStore(char* buf, size_t maxSize, size_t* len);
static inline void        FormatCapturedTimestamp(struct SolidSyslogFormatter* f, const struct SolidSyslogTimestamp* ts);
static inline void        FormatMessage(struct SolidSyslogFormatter* f, const struct SolidSyslogMessage* message);
static inline void        FormatMsg(struct SolidSyslogFormatter* f, const char* msg);
static inline void        FormatMsgId(struct SolidSyslogFormatter* f, const char* messageId);
static inline void        FormatNilvalue(struct SolidSyslogFormatter* f);
static inline void        FormatNonZeroUtcOffset(struct SolidSyslogFormatter* f, int16_t offsetMinutes);
static inline void        FormatPrival(struct SolidSyslogFormatter* f, uint8_t prival);
static inline void        FormatStringField(struct SolidSyslogFormatter* f, SolidSyslogStringFunction fn, size_t maxSize);
static inline void        FormatStructuredData(struct SolidSyslogFormatter* f, struct SolidSyslogStructuredData** sd, size_t sdCount);
static inline void        FormatTimestamp(struct SolidSyslogFormatter* f, SolidSyslogClockFunction clock);
static inline void        FormatUtcOffset(struct SolidSyslogFormatter* f, int16_t offsetMinutes);
static inline bool        IsServiceEnabled(void);
static inline uint8_t     MakePrival(const struct SolidSyslogMessage* message);
static void               NilClock(struct SolidSyslogTimestamp* ts);
static void               NilStringFunction(struct SolidSyslogFormatter* formatter);
static inline bool        PrivalComponentsAreValid(uint8_t facility, uint8_t severity);
static void               ProcessMessages(void);
static inline bool        ReceiveFromBufferIntoStore(char* buf, size_t maxSize, size_t* len);
static inline bool        SeverityIsValid(uint8_t severity);
static inline const char* SkipLeadingBom(const char* msg);
static inline bool        StringIsValid(const char* value);
static inline bool        TimestampIsValid(const struct SolidSyslogTimestamp* ts);

struct SolidSyslog
{
    struct SolidSyslogBuffer*          buffer;
    struct SolidSyslogSender*          sender;
    SolidSyslogClockFunction           clock;
    SolidSyslogStringFunction          getHostname;
    SolidSyslogStringFunction          getAppName;
    SolidSyslogStringFunction          getProcessId;
    struct SolidSyslogStore*           store;
    struct SolidSyslogStructuredData** sd;
    size_t                             sdCount;
};

static struct SolidSyslog instance = {
    .clock        = NilClock,
    .getHostname  = NilStringFunction,
    .getAppName   = NilStringFunction,
    .getProcessId = NilStringFunction,
};

static void NilClock(struct SolidSyslogTimestamp* ts)
{
    (void) ts;
}

static void NilStringFunction(struct SolidSyslogFormatter* formatter)
{
    (void) formatter;
}

void SolidSyslog_Create(const struct SolidSyslogConfig* config)
{
    instance.buffer = config->buffer;
    instance.sender = config->sender;
    ASSIGN_IF_NON_NULL(instance.clock, config->clock);
    ASSIGN_IF_NON_NULL(instance.getHostname, config->getHostname);
    ASSIGN_IF_NON_NULL(instance.getAppName, config->getAppName);
    ASSIGN_IF_NON_NULL(instance.getProcessId, config->getProcessId);
    instance.store   = config->store;
    instance.sd      = config->sd;
    instance.sdCount = config->sdCount;
}

void SolidSyslog_Destroy(void)
{
    instance.buffer       = NULL;
    instance.sender       = NULL;
    instance.clock        = NilClock;
    instance.getHostname  = NilStringFunction;
    instance.getAppName   = NilStringFunction;
    instance.getProcessId = NilStringFunction;
    instance.store        = NULL;
    instance.sd           = NULL;
    instance.sdCount      = 0;
}

void SolidSyslog_Service(void)
{
    if (IsServiceEnabled())
    {
        ProcessMessages();
    }
}

static inline bool IsServiceEnabled(void)
{
    return !SolidSyslogStore_IsHalted(instance.store);
}

static void ProcessMessages(void)
{
    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t len = 0;

    bool haveMessage = ReceiveFromBufferIntoStore(buf, sizeof(buf), &len);
    bool fromStore   = FetchFromStore(buf, sizeof(buf), &len);

    if (fromStore || haveMessage)
    {
        if (SolidSyslogSender_Send(instance.sender, buf, len))
        {
            if (fromStore)
            {
                SolidSyslogStore_MarkSent(instance.store);
            }
        }
    }
}

static inline bool ReceiveFromBufferIntoStore(char* buf, size_t maxSize, size_t* len)
{
    bool received = SolidSyslogBuffer_Read(instance.buffer, buf, maxSize, len);

    if (received)
    {
        SolidSyslogStore_Write(instance.store, buf, *len);
    }

    return received;
}

static inline bool FetchFromStore(char* buf, size_t maxSize, size_t* len)
{
    if (SolidSyslogStore_HasUnsent(instance.store))
    {
        return SolidSyslogStore_ReadNextUnsent(instance.store, buf, maxSize, len);
    }

    return false;
}

void SolidSyslog_Log(const struct SolidSyslogMessage* message)
{
    SolidSyslogFormatterStorage  storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_MESSAGE_SIZE)];
    struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(storage, SOLIDSYSLOG_MAX_MESSAGE_SIZE);

    FormatMessage(f, message);
    SolidSyslogBuffer_Write(instance.buffer, SolidSyslogFormatter_AsFormattedBuffer(f), SolidSyslogFormatter_Length(f));
}

static inline void FormatMessage(struct SolidSyslogFormatter* f, const struct SolidSyslogMessage* message)
{
    FormatPrival(f, MakePrival(message));
    SolidSyslogFormatter_AsciiCharacter(f, '1');
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    FormatTimestamp(f, instance.clock);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    FormatStringField(f, instance.getHostname, SOLIDSYSLOG_MAX_HOSTNAME_SIZE);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    FormatStringField(f, instance.getAppName, SOLIDSYSLOG_MAX_APP_NAME_SIZE);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    FormatStringField(f, instance.getProcessId, SOLIDSYSLOG_MAX_PROCESS_ID_SIZE);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    FormatMsgId(f, message->messageId);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    FormatStructuredData(f, instance.sd, instance.sdCount);
    FormatMsg(f, message->msg);
}

static inline void FormatPrival(struct SolidSyslogFormatter* f, uint8_t prival)
{
    SolidSyslogFormatter_AsciiCharacter(f, '<');
    SolidSyslogFormatter_Uint32(f, prival);
    SolidSyslogFormatter_AsciiCharacter(f, '>');
}

static inline uint8_t MakePrival(const struct SolidSyslogMessage* message)
{
    uint8_t f      = (uint8_t) message->facility;
    uint8_t s      = (uint8_t) message->severity;
    uint8_t prival = CombineFacilityAndSeverity(SOLIDSYSLOG_FACILITY_SYSLOG, SOLIDSYSLOG_SEVERITY_ERR);

    if (PrivalComponentsAreValid(f, s))
    {
        prival = CombineFacilityAndSeverity(f, s);
    }

    return prival;
}

static inline uint8_t CombineFacilityAndSeverity(uint8_t facility, uint8_t severity)
{
    return (uint8_t) ((facility * UINT8_C(8)) + severity);
}

static inline bool PrivalComponentsAreValid(uint8_t facility, uint8_t severity)
{
    return FacilityIsValid(facility) && SeverityIsValid(severity);
}

static inline bool FacilityIsValid(uint8_t facility)
{
    return facility <= SOLIDSYSLOG_FACILITY_LOCAL7;
}

static inline bool SeverityIsValid(uint8_t severity)
{
    return severity <= SOLIDSYSLOG_SEVERITY_DEBUG;
}

static inline void FormatTimestamp(struct SolidSyslogFormatter* f, SolidSyslogClockFunction clock)
{
    struct SolidSyslogTimestamp ts = {0};

    if (CaptureTimestamp(&ts, clock))
    {
        FormatCapturedTimestamp(f, &ts);
    }
    else
    {
        FormatNilvalue(f);
    }
}

static inline bool CaptureTimestamp(struct SolidSyslogTimestamp* ts, SolidSyslogClockFunction clock)
{
    clock(ts);
    return TimestampIsValid(ts);
}

static inline bool TimestampIsValid(const struct SolidSyslogTimestamp* ts)
{
    bool valid = true;

    valid = valid && (ts->month >= 1U) && (ts->month <= 12U);
    valid = valid && (ts->day >= 1U) && (ts->day <= 31U);
    valid = valid && (ts->hour <= 23U);
    valid = valid && (ts->minute <= 59U);
    valid = valid && (ts->second <= 59U);
    valid = valid && (ts->microsecond <= 999999U);
    valid = valid && (ts->utcOffsetMinutes >= -720) && (ts->utcOffsetMinutes <= 840);

    return valid;
}

static inline void FormatCapturedTimestamp(struct SolidSyslogFormatter* f, const struct SolidSyslogTimestamp* ts)
{
    SolidSyslogFormatter_FourDigit(f, ts->year);
    SolidSyslogFormatter_AsciiCharacter(f, '-');
    SolidSyslogFormatter_TwoDigit(f, ts->month);
    SolidSyslogFormatter_AsciiCharacter(f, '-');
    SolidSyslogFormatter_TwoDigit(f, ts->day);
    SolidSyslogFormatter_AsciiCharacter(f, 'T');
    SolidSyslogFormatter_TwoDigit(f, ts->hour);
    SolidSyslogFormatter_AsciiCharacter(f, ':');
    SolidSyslogFormatter_TwoDigit(f, ts->minute);
    SolidSyslogFormatter_AsciiCharacter(f, ':');
    SolidSyslogFormatter_TwoDigit(f, ts->second);
    SolidSyslogFormatter_AsciiCharacter(f, '.');
    SolidSyslogFormatter_SixDigit(f, ts->microsecond);
    FormatUtcOffset(f, ts->utcOffsetMinutes);
}

static inline void FormatUtcOffset(struct SolidSyslogFormatter* f, int16_t offsetMinutes)
{
    if (offsetMinutes == 0)
    {
        SolidSyslogFormatter_AsciiCharacter(f, 'Z');
    }
    else
    {
        FormatNonZeroUtcOffset(f, offsetMinutes);
    }
}

static inline void FormatNonZeroUtcOffset(struct SolidSyslogFormatter* f, int16_t offsetMinutes)
{
    int16_t absoluteMinutes = AbsoluteInt16(offsetMinutes);

    SolidSyslogFormatter_AsciiCharacter(f, (offsetMinutes > 0) ? '+' : '-');
    SolidSyslogFormatter_TwoDigit(f, (uint32_t) (absoluteMinutes / 60));
    SolidSyslogFormatter_AsciiCharacter(f, ':');
    SolidSyslogFormatter_TwoDigit(f, (uint32_t) (absoluteMinutes % 60));
}

static inline int16_t AbsoluteInt16(int16_t value)
{
    int16_t result = value;

    if (value < 0)
    {
        result = (int16_t) (-value);
    }

    return result;
}

static inline void FormatStringField(struct SolidSyslogFormatter* f, SolidSyslogStringFunction fn, size_t maxSize)
{
    SolidSyslogFormatterStorage  fieldStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_HOSTNAME_SIZE)];
    struct SolidSyslogFormatter* field = SolidSyslogFormatter_Create(fieldStorage, maxSize);

    fn(field);

    size_t fieldLength = SolidSyslogFormatter_Length(field);

    if (fieldLength > 0)
    {
        SolidSyslogFormatter_PrintUsAsciiString(f, SolidSyslogFormatter_AsFormattedBuffer(field), fieldLength);
    }
    else
    {
        FormatNilvalue(f);
    }
}

static inline void FormatMsgId(struct SolidSyslogFormatter* f, const char* messageId)
{
    size_t lengthBefore = SolidSyslogFormatter_Length(f);

    if (StringIsValid(messageId))
    {
        SolidSyslogFormatter_PrintUsAsciiString(f, messageId, SOLIDSYSLOG_MAX_MSGID_SIZE - 1);
    }

    if (SolidSyslogFormatter_Length(f) == lengthBefore)
    {
        FormatNilvalue(f);
    }
}

static inline bool StringIsValid(const char* value)
{
    return (value != NULL) && (value[0] != '\0');
}

static inline void FormatStructuredData(struct SolidSyslogFormatter* f, struct SolidSyslogStructuredData** sd, size_t sdCount)
{
    size_t lengthBefore = SolidSyslogFormatter_Length(f);

    for (size_t i = 0; i < sdCount; i++)
    {
        SolidSyslogStructuredData_Format(sd[i], f);
    }

    if (SolidSyslogFormatter_Length(f) == lengthBefore)
    {
        FormatNilvalue(f);
    }
}

static inline void FormatMsg(struct SolidSyslogFormatter* f, const char* msg)
{
    if (StringIsValid(msg))
    {
        SolidSyslogFormatter_AsciiCharacter(f, ' ');
        SolidSyslogFormatter_Bom(f);
        SolidSyslogFormatter_BoundedString(f, SkipLeadingBom(msg), SOLIDSYSLOG_MAX_MESSAGE_SIZE);
    }
}

/* MSG body MUST start with the UTF-8 BOM per RFC 5424 §6.4. We always
 * emit the BOM ourselves; if the caller already prefixed one, strip it
 * so the wire frame contains exactly one. */
static inline const char* SkipLeadingBom(const char* msg)
{
    if ((msg[0] == '\xEF') && (msg[1] == '\xBB') && (msg[2] == '\xBF'))
    {
        return msg + 3;
    }
    return msg;
}

static inline void FormatNilvalue(struct SolidSyslogFormatter* f)
{
    SolidSyslogFormatter_AsciiCharacter(f, '-');
}
