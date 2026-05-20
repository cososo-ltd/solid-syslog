#include "SolidSyslog.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogStringFunction.h"
#include "SolidSyslogStructuredData.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogFormatter;
struct SolidSyslogStructuredData;

enum
{
    SOLIDSYSLOG_MAX_APP_NAME_SIZE = 49,
    SOLIDSYSLOG_MAX_HOSTNAME_SIZE = 256,
    SOLIDSYSLOG_MAX_MSGID_SIZE = 33,
    SOLIDSYSLOG_MAX_PROCESS_ID_SIZE = 129
};

struct SolidSyslog
{
    struct SolidSyslogBuffer* Buffer;
    struct SolidSyslogSender* Sender;
    SolidSyslogClockFunction Clock;
    SolidSyslogStringFunction GetHostname;
    SolidSyslogStringFunction GetAppName;
    SolidSyslogStringFunction GetProcessId;
    struct SolidSyslogStore* Store;
    struct SolidSyslogStructuredData** Sd;
    size_t SdCount;
};

/* Default collaborators that make the singleton crash-safe pre-Create and
 * post-Destroy, and stand in when the integrator left a required slot NULL
 * in their config. Vtable dispatch never NULL-checks — these defaults make
 * every collaborator pointer safe to call. NullClock and NullStringFunction
 * are TU-local — no public Null equivalent exists for the function-pointer
 * typedefs. The Buffer/Sender/Store slots route through the public
 * SolidSyslogNull*_Get() siblings; the bad-config signal is the Create-time
 * NULL_BUFFER / NULL_SENDER / NULL_STORE error, not a runtime loud-once. */
static void SolidSyslog_NullClock(struct SolidSyslogTimestamp* ts);
static void SolidSyslog_NullStringFunction(struct SolidSyslogFormatter* formatter);
static void SolidSyslog_ResetInstanceToDefaults(void);
static inline void SolidSyslog_EnsureInstanceInitialised(void);

static bool instanceInitialised;
static bool instanceDefaultsLoaded;
static struct SolidSyslog instance;

/* SolidSyslog helpers forward-declared so the public functions and
 * _Create/_Destroy can call them; each is defined immediately beneath its
 * first caller below. */
static inline int16_t SolidSyslog_AbsoluteInt16(int16_t value);
static inline bool SolidSyslog_CaptureTimestamp(struct SolidSyslogTimestamp* ts, SolidSyslogClockFunction clock);
static inline uint8_t SolidSyslog_CombineFacilityAndSeverity(uint8_t facility, uint8_t severity);
static inline bool SolidSyslog_FacilityIsValid(uint8_t facility);
static inline void SolidSyslog_DrainBufferIntoStore(void);
static inline void SolidSyslog_SendOneFromStore(void);
static inline void SolidSyslog_FormatCapturedTimestamp(
    struct SolidSyslogFormatter* f,
    const struct SolidSyslogTimestamp* ts
);
static inline void SolidSyslog_FormatMessage(struct SolidSyslogFormatter* f, const struct SolidSyslogMessage* message);
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
static void SolidSyslog_InstallAppName(SolidSyslogStringFunction configured);
static void SolidSyslog_InstallBuffer(struct SolidSyslogBuffer* configured);
static void SolidSyslog_InstallClock(SolidSyslogClockFunction configured);
static void SolidSyslog_InstallConfig(const struct SolidSyslogConfig* config);
static void SolidSyslog_InstallHostname(SolidSyslogStringFunction configured);
static void SolidSyslog_InstallProcessId(SolidSyslogStringFunction configured);
static void SolidSyslog_InstallSender(struct SolidSyslogSender* configured);
static void SolidSyslog_InstallStore(struct SolidSyslogStore* configured);
static void SolidSyslog_InstallStructuredData(struct SolidSyslogStructuredData** configured, size_t count);
static inline bool SolidSyslog_IsServiceEnabled(void);
static inline uint8_t SolidSyslog_MakePrival(const struct SolidSyslogMessage* message);
static inline bool SolidSyslog_PrivalComponentsAreValid(uint8_t facility, uint8_t severity);
static void SolidSyslog_ProcessMessages(void);
static inline bool SolidSyslog_SeverityIsValid(uint8_t severity);
static inline const char* SolidSyslog_SkipLeadingBom(const char* msg);
static inline bool SolidSyslog_StringIsValid(const char* value);
static inline bool SolidSyslog_TimestampIsValid(const struct SolidSyslogTimestamp* ts);

void SolidSyslog_Create(const struct SolidSyslogConfig* config)
{
    SolidSyslog_EnsureInstanceInitialised();
    if (instanceInitialised)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_CREATE_ALREADY_INITIALISED);
    }
    else if (config == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_CONFIG);
    }
    else
    {
        SolidSyslog_InstallConfig(config);
        instanceInitialised = true;
    }
}

static inline void SolidSyslog_EnsureInstanceInitialised(void)
{
    if (!instanceDefaultsLoaded)
    {
        SolidSyslog_ResetInstanceToDefaults();
        instanceDefaultsLoaded = true;
    }
}

static void SolidSyslog_ResetInstanceToDefaults(void)
{
    instance.Buffer = SolidSyslogNullBuffer_Get();
    instance.Sender = SolidSyslogNullSender_Get();
    instance.Store = SolidSyslogNullStore_Get();
    instance.Clock = SolidSyslog_NullClock;
    instance.GetHostname = SolidSyslog_NullStringFunction;
    instance.GetAppName = SolidSyslog_NullStringFunction;
    instance.GetProcessId = SolidSyslog_NullStringFunction;
    instance.Sd = NULL;
    instance.SdCount = 0;
}

static void SolidSyslog_InstallConfig(const struct SolidSyslogConfig* config)
{
    SolidSyslog_ResetInstanceToDefaults();
    SolidSyslog_InstallBuffer(config->Buffer);
    SolidSyslog_InstallSender(config->Sender);
    SolidSyslog_InstallStore(config->Store);
    SolidSyslog_InstallClock(config->Clock);
    SolidSyslog_InstallHostname(config->GetHostname);
    SolidSyslog_InstallAppName(config->GetAppName);
    SolidSyslog_InstallProcessId(config->GetProcessId);
    SolidSyslog_InstallStructuredData(config->Sd, config->SdCount);
}

static void SolidSyslog_InstallBuffer(struct SolidSyslogBuffer* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_BUFFER);
    }
    else
    {
        instance.Buffer = configured;
    }
}

static void SolidSyslog_InstallSender(struct SolidSyslogSender* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_SENDER);
    }
    else
    {
        instance.Sender = configured;
    }
}

static void SolidSyslog_InstallStore(struct SolidSyslogStore* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_STORE);
    }
    else
    {
        instance.Store = configured;
    }
}

static void SolidSyslog_InstallClock(SolidSyslogClockFunction configured)
{
    if (configured != NULL)
    {
        instance.Clock = configured;
    }
}

static void SolidSyslog_InstallHostname(SolidSyslogStringFunction configured)
{
    if (configured != NULL)
    {
        instance.GetHostname = configured;
    }
}

static void SolidSyslog_InstallAppName(SolidSyslogStringFunction configured)
{
    if (configured != NULL)
    {
        instance.GetAppName = configured;
    }
}

static void SolidSyslog_InstallProcessId(SolidSyslogStringFunction configured)
{
    if (configured != NULL)
    {
        instance.GetProcessId = configured;
    }
}

static void SolidSyslog_InstallStructuredData(struct SolidSyslogStructuredData** configured, size_t count)
{
    instance.Sd = configured;
    instance.SdCount = count;
}

void SolidSyslog_Destroy(void)
{
    SolidSyslog_ResetInstanceToDefaults();
    instanceDefaultsLoaded = true;
    instanceInitialised = false;
}

void SolidSyslog_Service(void)
{
    SolidSyslog_EnsureInstanceInitialised();
    if (SolidSyslog_IsServiceEnabled())
    {
        SolidSyslog_ProcessMessages();
    }
}

static inline bool SolidSyslog_IsServiceEnabled(void)
{
    return !SolidSyslogStore_IsHalted(instance.Store);
}

static void SolidSyslog_ProcessMessages(void)
{
    SolidSyslog_DrainBufferIntoStore();
    SolidSyslog_SendOneFromStore();
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
static inline void SolidSyslog_DrainBufferIntoStore(void)
{
    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t len = 0;

    while (SolidSyslogBuffer_Read(instance.Buffer, buf, sizeof(buf), &len))
    {
        if (!SolidSyslogStore_Write(instance.Store, buf, len) && SolidSyslogStore_IsTransient(instance.Store))
        {
            SolidSyslogSender_Send(instance.Sender, buf, len);
        }
    }
}

static inline void SolidSyslog_SendOneFromStore(void)
{
    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t len = 0;

    if (SolidSyslogStore_ReadNextUnsent(instance.Store, buf, sizeof(buf), &len) &&
        SolidSyslogSender_Send(instance.Sender, buf, len))
    {
        SolidSyslogStore_MarkSent(instance.Store);
    }
}

void SolidSyslog_Log(const struct SolidSyslogMessage* message)
{
    SolidSyslog_EnsureInstanceInitialised();
    if (message == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_LOG_NULL_MESSAGE);
    }
    else
    {
        SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_MESSAGE_SIZE)];
        struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(storage, SOLIDSYSLOG_MAX_MESSAGE_SIZE);

        SolidSyslog_FormatMessage(f, message);
        SolidSyslogBuffer_Write(
            instance.Buffer,
            SolidSyslogFormatter_AsFormattedBuffer(f),
            SolidSyslogFormatter_Length(f)
        );
    }
}

static inline void SolidSyslog_FormatMessage(struct SolidSyslogFormatter* f, const struct SolidSyslogMessage* message)
{
    SolidSyslog_FormatPrival(f, SolidSyslog_MakePrival(message));
    SolidSyslogFormatter_AsciiCharacter(f, '1');
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    SolidSyslog_FormatTimestamp(f, instance.Clock);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    SolidSyslog_FormatStringField(f, instance.GetHostname, SOLIDSYSLOG_MAX_HOSTNAME_SIZE);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    SolidSyslog_FormatStringField(f, instance.GetAppName, SOLIDSYSLOG_MAX_APP_NAME_SIZE);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    SolidSyslog_FormatStringField(f, instance.GetProcessId, SOLIDSYSLOG_MAX_PROCESS_ID_SIZE);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    SolidSyslog_FormatMsgId(f, message->MessageId);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    SolidSyslog_FormatStructuredData(f, instance.Sd, instance.SdCount);
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
    if ((bytes[0] == 0xEFU) && (bytes[1] == 0xBBU) && (bytes[2] == 0xBFU))
    {
        return msg + 3;
    }
    return msg;
}

static inline void SolidSyslog_FormatNilvalue(struct SolidSyslogFormatter* f)
{
    SolidSyslogFormatter_AsciiCharacter(f, '-');
}

/* Default function-pointer callbacks. No public Null equivalent exists for
 * the SolidSyslogClockFunction / SolidSyslogStringFunction typedefs, so these
 * stay TU-local. Buffer/Sender/Store defaults route through the public
 * SolidSyslogNull{Buffer,Sender,Store}_Get() siblings via
 * SolidSyslog_ResetInstanceToDefaults above. */

static void SolidSyslog_NullClock(struct SolidSyslogTimestamp* ts)
{
    (void) ts;
}

static void SolidSyslog_NullStringFunction(struct SolidSyslogFormatter* formatter)
{
    (void) formatter;
}
