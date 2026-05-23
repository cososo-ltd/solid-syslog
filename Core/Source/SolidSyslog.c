#include "SolidSyslog.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPrivate.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogStringFunction.h"
#include "SolidSyslogStructuredData.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogBuffer;
struct SolidSyslogFormatter;
struct SolidSyslogSender;
struct SolidSyslogStore;
struct SolidSyslogStructuredData;

enum
{
    SOLIDSYSLOG_MAX_APP_NAME_SIZE = 49,
    SOLIDSYSLOG_MAX_HOSTNAME_SIZE = 256,
    SOLIDSYSLOG_MAX_MSGID_SIZE = 33,
    SOLIDSYSLOG_MAX_PROCESS_ID_SIZE = 129
};

static inline int16_t SolidSyslog_AbsoluteInt16(int16_t value);
static inline bool SolidSyslog_CaptureTimestamp(struct SolidSyslogTimestamp* ts, SolidSyslogClockFunction clock);
static inline uint8_t SolidSyslog_CombineFacilityAndSeverity(uint8_t facility, uint8_t severity);
static inline bool SolidSyslog_FacilityIsValid(uint8_t facility);
static inline void SolidSyslog_DrainBufferIntoStore(struct SolidSyslog* self);
static inline void SolidSyslog_SendOneFromStore(struct SolidSyslog* self);
static inline void SolidSyslog_FormatCapturedTimestamp(
    struct SolidSyslogFormatter* f,
    const struct SolidSyslogTimestamp* ts
);
static inline void SolidSyslog_FormatMessage(
    struct SolidSyslogFormatter* f,
    struct SolidSyslog* self,
    const struct SolidSyslogMessage* message
);
static inline void SolidSyslog_FormatMsg(struct SolidSyslogFormatter* f, const char* msg);
static inline void SolidSyslog_FormatMsgId(struct SolidSyslogFormatter* f, const char* messageId);
static inline void SolidSyslog_FormatNilvalue(struct SolidSyslogFormatter* f);
static inline void SolidSyslog_FormatNonZeroUtcOffset(struct SolidSyslogFormatter* f, int16_t offsetMinutes);
static inline void SolidSyslog_FormatPrival(struct SolidSyslogFormatter* f, uint8_t prival);
static inline void SolidSyslog_FormatStringField(
    struct SolidSyslogFormatter* f,
    SolidSyslogStringFunction fn,
    size_t maxSize
);
static inline void SolidSyslog_FormatStructuredData(
    struct SolidSyslogFormatter* f,
    struct SolidSyslogStructuredData** sd,
    size_t sdCount
);
static inline void SolidSyslog_FormatTimestamp(struct SolidSyslogFormatter* f, SolidSyslogClockFunction clock);
static inline void SolidSyslog_FormatUtcOffset(struct SolidSyslogFormatter* f, int16_t offsetMinutes);
static void SolidSyslog_InstallAppName(struct SolidSyslog* self, SolidSyslogStringFunction configured);
static void SolidSyslog_InstallBuffer(struct SolidSyslog* self, struct SolidSyslogBuffer* configured);
static void SolidSyslog_InstallClock(struct SolidSyslog* self, SolidSyslogClockFunction configured);
static void SolidSyslog_InstallHostname(struct SolidSyslog* self, SolidSyslogStringFunction configured);
static void SolidSyslog_InstallProcessId(struct SolidSyslog* self, SolidSyslogStringFunction configured);
static void SolidSyslog_InstallSender(struct SolidSyslog* self, struct SolidSyslogSender* configured);
static void SolidSyslog_InstallStore(struct SolidSyslog* self, struct SolidSyslogStore* configured);
static void SolidSyslog_InstallStructuredData(
    struct SolidSyslog* self,
    struct SolidSyslogStructuredData** configured,
    size_t count
);
static inline bool SolidSyslog_IsServiceEnabled(struct SolidSyslog* self);
static inline uint8_t SolidSyslog_MakePrival(const struct SolidSyslogMessage* message);
static inline bool SolidSyslog_PrivalComponentsAreValid(uint8_t facility, uint8_t severity);
static void SolidSyslog_ProcessMessages(struct SolidSyslog* self);
static void SolidSyslog_ResetToDefaults(struct SolidSyslog* self);
static inline bool SolidSyslog_SeverityIsValid(uint8_t severity);
static inline const char* SolidSyslog_SkipLeadingBom(const char* msg);
static inline bool SolidSyslog_StringIsValid(const char* value);
static inline bool SolidSyslog_TimestampIsValid(const struct SolidSyslogTimestamp* ts);

void SolidSyslog_Initialise(struct SolidSyslog* self, const struct SolidSyslogConfig* config)
{
    SolidSyslog_ResetToDefaults(self);
    SolidSyslog_InstallBuffer(self, config->Buffer);
    SolidSyslog_InstallSender(self, config->Sender);
    SolidSyslog_InstallStore(self, config->Store);
    SolidSyslog_InstallClock(self, config->Clock);
    SolidSyslog_InstallHostname(self, config->GetHostname);
    SolidSyslog_InstallAppName(self, config->GetAppName);
    SolidSyslog_InstallProcessId(self, config->GetProcessId);
    SolidSyslog_InstallStructuredData(self, config->Sd, config->SdCount);
}

void SolidSyslog_Cleanup(struct SolidSyslog* self)
{
    /* Reset to safe defaults so a stale-handle Log/Service after Destroy is a
     * silent no-op rather than a NULL-fn-pointer crash. The slot is then
     * re-claimable by the next _Create. */
    SolidSyslog_ResetToDefaults(self);
}

static void SolidSyslog_ResetToDefaults(struct SolidSyslog* self)
{
    self->Buffer = SolidSyslogNullBuffer_Get();
    self->Sender = SolidSyslogNullSender_Get();
    self->Store = SolidSyslogNullStore_Get();
    self->Clock = SolidSyslog_NullClock;
    self->GetHostname = SolidSyslog_NullStringFunction;
    self->GetAppName = SolidSyslog_NullStringFunction;
    self->GetProcessId = SolidSyslog_NullStringFunction;
    self->Sd = NULL;
    self->SdCount = 0;
}

static void SolidSyslog_InstallBuffer(struct SolidSyslog* self, struct SolidSyslogBuffer* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &SolidSyslogErrorSource,
            (uint8_t) SOLIDSYSLOG_ERROR_CREATE_NULL_BUFFER
        );
    }
    else
    {
        self->Buffer = configured;
    }
}

static void SolidSyslog_InstallSender(struct SolidSyslog* self, struct SolidSyslogSender* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &SolidSyslogErrorSource,
            (uint8_t) SOLIDSYSLOG_ERROR_CREATE_NULL_SENDER
        );
    }
    else
    {
        self->Sender = configured;
    }
}

static void SolidSyslog_InstallStore(struct SolidSyslog* self, struct SolidSyslogStore* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &SolidSyslogErrorSource,
            (uint8_t) SOLIDSYSLOG_ERROR_CREATE_NULL_STORE
        );
    }
    else
    {
        self->Store = configured;
    }
}

static void SolidSyslog_InstallClock(struct SolidSyslog* self, SolidSyslogClockFunction configured)
{
    if (configured != NULL)
    {
        self->Clock = configured;
    }
}

static void SolidSyslog_InstallHostname(struct SolidSyslog* self, SolidSyslogStringFunction configured)
{
    if (configured != NULL)
    {
        self->GetHostname = configured;
    }
}

static void SolidSyslog_InstallAppName(struct SolidSyslog* self, SolidSyslogStringFunction configured)
{
    if (configured != NULL)
    {
        self->GetAppName = configured;
    }
}

static void SolidSyslog_InstallProcessId(struct SolidSyslog* self, SolidSyslogStringFunction configured)
{
    if (configured != NULL)
    {
        self->GetProcessId = configured;
    }
}

static void SolidSyslog_InstallStructuredData(
    struct SolidSyslog* self,
    struct SolidSyslogStructuredData** configured,
    size_t count
)
{
    self->Sd = configured;
    self->SdCount = count;
}

void SolidSyslog_Service(struct SolidSyslog* handle)
{
    if (handle == NULL)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &SolidSyslogErrorSource,
            (uint8_t) SOLIDSYSLOG_ERROR_SERVICE_NULL_HANDLE
        );
    }
    else if (SolidSyslog_IsServiceEnabled(handle))
    {
        SolidSyslog_ProcessMessages(handle);
    }
    else
    {
        /* Halted store — skip drain/send. */
    }
}

static inline bool SolidSyslog_IsServiceEnabled(struct SolidSyslog* self)
{
    return !SolidSyslogStore_IsHalted(self->Store);
}

static void SolidSyslog_ProcessMessages(struct SolidSyslog* self)
{
    SolidSyslog_DrainBufferIntoStore(self);
    SolidSyslog_SendOneFromStore(self);
}

/* Eagerly drain the buffer so the producer-side shock absorber stays small while
 * the sender is slow or down — overflow then engages the store's discard policy
 * rather than silently dropping at the buffer. The fall-through to a direct
 * Sender_Send on Store_Write rejection is *only* taken when the store is
 * transient (NullStore): a NullStore Write rejection means "I never retain
 * anything, please try the sender." For a real BlockStore, rejection is the
 * discard policy speaking — letting that message escape via direct send would
 * break the discard-newest contract (a newer message would bypass older stored
 * ones once the sender recovered). */
static inline void SolidSyslog_DrainBufferIntoStore(struct SolidSyslog* self)
{
    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t len = 0;

    while (SolidSyslogBuffer_Read(self->Buffer, buf, sizeof(buf), &len))
    {
        if (!SolidSyslogStore_Write(self->Store, buf, len) && SolidSyslogStore_IsTransient(self->Store))
        {
            (void) SolidSyslogSender_Send(self->Sender, buf, len);
        }
    }
}

static inline void SolidSyslog_SendOneFromStore(struct SolidSyslog* self)
{
    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t len = 0;

    if (SolidSyslogStore_ReadNextUnsent(self->Store, buf, sizeof(buf), &len) &&
        SolidSyslogSender_Send(self->Sender, buf, len))
    {
        SolidSyslogStore_MarkSent(self->Store);
    }
}

void SolidSyslog_Log(struct SolidSyslog* handle, const struct SolidSyslogMessage* message)
{
    if (handle == NULL)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &SolidSyslogErrorSource,
            (uint8_t) SOLIDSYSLOG_ERROR_LOG_NULL_HANDLE
        );
    }
    else if (message == NULL)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &SolidSyslogErrorSource,
            (uint8_t) SOLIDSYSLOG_ERROR_LOG_NULL_MESSAGE
        );
    }
    else
    {
        SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_MESSAGE_SIZE)];
        struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(storage, SOLIDSYSLOG_MAX_MESSAGE_SIZE);

        SolidSyslog_FormatMessage(f, handle, message);
        SolidSyslogBuffer_Write(
            handle->Buffer,
            SolidSyslogFormatter_AsFormattedBuffer(f),
            SolidSyslogFormatter_Length(f)
        );
    }
}

static inline void SolidSyslog_FormatMessage(
    struct SolidSyslogFormatter* f,
    struct SolidSyslog* self,
    const struct SolidSyslogMessage* message
)
{
    SolidSyslog_FormatPrival(f, SolidSyslog_MakePrival(message));
    SolidSyslogFormatter_AsciiCharacter(f, '1');
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    SolidSyslog_FormatTimestamp(f, self->Clock);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    SolidSyslog_FormatStringField(f, self->GetHostname, SOLIDSYSLOG_MAX_HOSTNAME_SIZE);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    SolidSyslog_FormatStringField(f, self->GetAppName, SOLIDSYSLOG_MAX_APP_NAME_SIZE);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    SolidSyslog_FormatStringField(f, self->GetProcessId, SOLIDSYSLOG_MAX_PROCESS_ID_SIZE);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    SolidSyslog_FormatMsgId(f, message->MessageId);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    SolidSyslog_FormatStructuredData(f, self->Sd, self->SdCount);
    SolidSyslog_FormatMsg(f, message->Msg);
}

static inline void SolidSyslog_FormatPrival(struct SolidSyslogFormatter* f, uint8_t prival)
{
    SolidSyslogFormatter_AsciiCharacter(f, '<');
    SolidSyslogFormatter_Uint32(f, prival);
    SolidSyslogFormatter_AsciiCharacter(f, '>');
}

static inline uint8_t SolidSyslog_MakePrival(const struct SolidSyslogMessage* message)
{
    uint8_t f = (uint8_t) message->Facility;
    uint8_t s = (uint8_t) message->Severity;
    uint8_t prival = SolidSyslog_CombineFacilityAndSeverity(SOLIDSYSLOG_FACILITY_SYSLOG, SOLIDSYSLOG_SEVERITY_ERROR);

    if (SolidSyslog_PrivalComponentsAreValid(f, s))
    {
        prival = SolidSyslog_CombineFacilityAndSeverity(f, s);
    }

    return prival;
}

static inline uint8_t SolidSyslog_CombineFacilityAndSeverity(uint8_t facility, uint8_t severity)
{
    return (uint8_t) ((facility * UINT8_C(8)) + severity);
}

static inline bool SolidSyslog_PrivalComponentsAreValid(uint8_t facility, uint8_t severity)
{
    return SolidSyslog_FacilityIsValid(facility) && SolidSyslog_SeverityIsValid(severity);
}

static inline bool SolidSyslog_FacilityIsValid(uint8_t facility)
{
    return facility <= (uint8_t) SOLIDSYSLOG_FACILITY_LOCAL7;
}

static inline bool SolidSyslog_SeverityIsValid(uint8_t severity)
{
    return severity <= (uint8_t) SOLIDSYSLOG_SEVERITY_DEBUG;
}

static inline void SolidSyslog_FormatTimestamp(struct SolidSyslogFormatter* f, SolidSyslogClockFunction clock)
{
    struct SolidSyslogTimestamp ts = {0};

    if (SolidSyslog_CaptureTimestamp(&ts, clock))
    {
        SolidSyslog_FormatCapturedTimestamp(f, &ts);
    }
    else
    {
        SolidSyslog_FormatNilvalue(f);
    }
}

static inline bool SolidSyslog_CaptureTimestamp(struct SolidSyslogTimestamp* ts, SolidSyslogClockFunction clock)
{
    clock(ts);
    return SolidSyslog_TimestampIsValid(ts);
}

static inline bool SolidSyslog_TimestampIsValid(const struct SolidSyslogTimestamp* ts)
{
    bool valid = true;

    valid = valid && (ts->Month >= 1U) && (ts->Month <= 12U);
    valid = valid && (ts->Day >= 1U) && (ts->Day <= 31U);
    valid = valid && (ts->Hour <= 23U);
    valid = valid && (ts->Minute <= 59U);
    valid = valid && (ts->Second <= 59U);
    valid = valid && (ts->Microsecond <= 999999U);
    valid = valid && (ts->UtcOffsetMinutes >= -720) && (ts->UtcOffsetMinutes <= 840);

    return valid;
}

static inline void SolidSyslog_FormatCapturedTimestamp(
    struct SolidSyslogFormatter* f,
    const struct SolidSyslogTimestamp* ts
)
{
    SolidSyslogFormatter_FourDigit(f, ts->Year);
    SolidSyslogFormatter_AsciiCharacter(f, '-');
    SolidSyslogFormatter_TwoDigit(f, ts->Month);
    SolidSyslogFormatter_AsciiCharacter(f, '-');
    SolidSyslogFormatter_TwoDigit(f, ts->Day);
    SolidSyslogFormatter_AsciiCharacter(f, 'T');
    SolidSyslogFormatter_TwoDigit(f, ts->Hour);
    SolidSyslogFormatter_AsciiCharacter(f, ':');
    SolidSyslogFormatter_TwoDigit(f, ts->Minute);
    SolidSyslogFormatter_AsciiCharacter(f, ':');
    SolidSyslogFormatter_TwoDigit(f, ts->Second);
    SolidSyslogFormatter_AsciiCharacter(f, '.');
    SolidSyslogFormatter_SixDigit(f, ts->Microsecond);
    SolidSyslog_FormatUtcOffset(f, ts->UtcOffsetMinutes);
}

static inline void SolidSyslog_FormatUtcOffset(struct SolidSyslogFormatter* f, int16_t offsetMinutes)
{
    if (offsetMinutes == 0)
    {
        SolidSyslogFormatter_AsciiCharacter(f, 'Z');
    }
    else
    {
        SolidSyslog_FormatNonZeroUtcOffset(f, offsetMinutes);
    }
}

static inline void SolidSyslog_FormatNonZeroUtcOffset(struct SolidSyslogFormatter* f, int16_t offsetMinutes)
{
    uint32_t absoluteMinutes = (uint32_t) SolidSyslog_AbsoluteInt16(offsetMinutes);

    SolidSyslogFormatter_AsciiCharacter(f, (offsetMinutes > 0) ? '+' : '-');
    SolidSyslogFormatter_TwoDigit(f, absoluteMinutes / 60U);
    SolidSyslogFormatter_AsciiCharacter(f, ':');
    SolidSyslogFormatter_TwoDigit(f, absoluteMinutes % 60U);
}

static inline int16_t SolidSyslog_AbsoluteInt16(int16_t value)
{
    int16_t result = value;

    if (value < 0)
    {
        result = (int16_t) (-value);
    }

    return result;
}

static inline void SolidSyslog_FormatStringField(
    struct SolidSyslogFormatter* f,
    SolidSyslogStringFunction fn,
    size_t maxSize
)
{
    SolidSyslogFormatterStorage fieldStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_HOSTNAME_SIZE)];
    struct SolidSyslogFormatter* field = SolidSyslogFormatter_Create(fieldStorage, maxSize);

    fn(field);

    size_t fieldLength = SolidSyslogFormatter_Length(field);

    if (fieldLength > 0U)
    {
        SolidSyslogFormatter_PrintUsAsciiString(f, SolidSyslogFormatter_AsFormattedBuffer(field), fieldLength);
    }
    else
    {
        SolidSyslog_FormatNilvalue(f);
    }
}

static inline void SolidSyslog_FormatMsgId(struct SolidSyslogFormatter* f, const char* messageId)
{
    size_t lengthBefore = SolidSyslogFormatter_Length(f);

    if (SolidSyslog_StringIsValid(messageId))
    {
        SolidSyslogFormatter_PrintUsAsciiString(f, messageId, SOLIDSYSLOG_MAX_MSGID_SIZE - 1);
    }

    if (SolidSyslogFormatter_Length(f) == lengthBefore)
    {
        SolidSyslog_FormatNilvalue(f);
    }
}

static inline bool SolidSyslog_StringIsValid(const char* value)
{
    return (value != NULL) && (value[0] != '\0');
}

static inline void SolidSyslog_FormatStructuredData(
    struct SolidSyslogFormatter* f,
    struct SolidSyslogStructuredData** sd,
    size_t sdCount
)
{
    size_t lengthBefore = SolidSyslogFormatter_Length(f);

    for (size_t i = 0; i < sdCount; i++)
    {
        SolidSyslogStructuredData_Format(sd[i], f);
    }

    if (SolidSyslogFormatter_Length(f) == lengthBefore)
    {
        SolidSyslog_FormatNilvalue(f);
    }
}

static inline void SolidSyslog_FormatMsg(struct SolidSyslogFormatter* f, const char* msg)
{
    if (SolidSyslog_StringIsValid(msg))
    {
        SolidSyslogFormatter_AsciiCharacter(f, ' ');
        SolidSyslogFormatter_Bom(f);
        SolidSyslogFormatter_BoundedString(f, SolidSyslog_SkipLeadingBom(msg), SOLIDSYSLOG_MAX_MESSAGE_SIZE);
    }
}

/* MSG body MUST start with the UTF-8 BOM per RFC 5424 §6.4. We always
 * emit the BOM ourselves; if the caller already prefixed one, strip it
 * so the wire frame contains exactly one. */
static inline const char* SolidSyslog_SkipLeadingBom(const char* msg)
{
    const unsigned char* bytes = (const unsigned char*) msg;
    const char* result = msg;
    if ((bytes[0] == 0xEFU) && (bytes[1] == 0xBBU) && (bytes[2] == 0xBFU))
    {
        result = &msg[3];
    }
    return result;
}

static inline void SolidSyslog_FormatNilvalue(struct SolidSyslogFormatter* f)
{
    SolidSyslogFormatter_AsciiCharacter(f, '-');
}

void SolidSyslog_NullClock(struct SolidSyslogTimestamp* ts)
{
    (void) ts;
}

void SolidSyslog_NullStringFunction(struct SolidSyslogFormatter* formatter)
{
    (void) formatter;
}
