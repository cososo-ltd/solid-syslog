#include "SolidSyslog.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBuffer.h"
#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogStoreDefinition.h"
#include "SolidSyslogStringFunction.h"
#include "SolidSyslogStructuredData.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogFormatter;
struct SolidSyslogStructuredData;

enum
{
    SOLIDSYSLOG_MAX_APP_NAME_SIZE   = 49,
    SOLIDSYSLOG_MAX_HOSTNAME_SIZE   = 256,
    SOLIDSYSLOG_MAX_MSGID_SIZE      = 33,
    SOLIDSYSLOG_MAX_PROCESS_ID_SIZE = 129
};

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

/* Forward declarations for the Nil "class" defined at the bottom of the file.
 * The Nil objects act as default collaborators that make the singleton
 * crash-safe pre-Create and post-Destroy; the file-static instance below and
 * the NilInstance template both reference these by address. The Nil
 * implementations live below SolidSyslog so this file reads as two cooperating
 * "classes" — SolidSyslog first, then the Nil collaborators it depends on. */
static void                     NilClock(struct SolidSyslogTimestamp* ts);
static void                     NilStringFunction(struct SolidSyslogFormatter* formatter);
static struct SolidSyslogBuffer NilBuffer;
static struct SolidSyslogSender NilSender;
static struct SolidSyslogStore  NilStore;
static bool                     nilBufferReportArmed;
static bool                     nilSenderReportArmed;
static bool                     instanceInitialised;

static struct SolidSyslog instance = {
    .buffer       = &NilBuffer,
    .sender       = &NilSender,
    .clock        = NilClock,
    .getHostname  = NilStringFunction,
    .getAppName   = NilStringFunction,
    .getProcessId = NilStringFunction,
    .store        = &NilStore,
};

/* Template used by _Create and _Destroy to reset every slot to its nil. C99
 * forbids initialising one file-static from another, so the same literal
 * appears once above and once here; both sites stay in sync trivially because
 * the values are nil-object addresses. */
static const struct SolidSyslog NilInstance = {
    .buffer       = &NilBuffer,
    .sender       = &NilSender,
    .clock        = NilClock,
    .getHostname  = NilStringFunction,
    .getAppName   = NilStringFunction,
    .getProcessId = NilStringFunction,
    .store        = &NilStore,
};

/* SolidSyslog helpers forward-declared so the public functions and
 * _Create/_Destroy can call them; each is defined immediately beneath its
 * first caller below. */
static inline int16_t     AbsoluteInt16(int16_t value);
static inline bool        CaptureTimestamp(struct SolidSyslogTimestamp* ts, SolidSyslogClockFunction clock);
static inline uint8_t     CombineFacilityAndSeverity(uint8_t facility, uint8_t severity);
static inline bool        FacilityIsValid(uint8_t facility);
static inline void        DrainBufferIntoStore(void);
static inline void        SendOneFromStore(void);
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
static void               InstallAppName(SolidSyslogStringFunction configured);
static void               InstallBuffer(struct SolidSyslogBuffer* configured);
static void               InstallClock(SolidSyslogClockFunction configured);
static void               InstallConfig(const struct SolidSyslogConfig* config);
static void               InstallHostname(SolidSyslogStringFunction configured);
static void               InstallProcessId(SolidSyslogStringFunction configured);
static void               InstallSender(struct SolidSyslogSender* configured);
static void               InstallStore(struct SolidSyslogStore* configured);
static void               InstallStructuredData(struct SolidSyslogStructuredData** configured, size_t count);
static inline bool        IsServiceEnabled(void);
static inline uint8_t     MakePrival(const struct SolidSyslogMessage* message);
static inline bool        PrivalComponentsAreValid(uint8_t facility, uint8_t severity);
static void               ProcessMessages(void);
static inline bool        SeverityIsValid(uint8_t severity);
static inline const char* SkipLeadingBom(const char* msg);
static inline bool        StringIsValid(const char* value);
static inline bool        TimestampIsValid(const struct SolidSyslogTimestamp* ts);

void SolidSyslog_Create(const struct SolidSyslogConfig* config)
{
    if (instanceInitialised)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_CREATE_ALREADY_INITIALISED);
    }
    else if (config == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_CONFIG);
    }
    else
    {
        InstallConfig(config);
        instanceInitialised = true;
    }
}

static void InstallConfig(const struct SolidSyslogConfig* config)
{
    instance = NilInstance;
    InstallBuffer(config->buffer);
    InstallSender(config->sender);
    InstallStore(config->store);
    InstallClock(config->clock);
    InstallHostname(config->getHostname);
    InstallAppName(config->getAppName);
    InstallProcessId(config->getProcessId);
    InstallStructuredData(config->sd, config->sdCount);
}

static void InstallBuffer(struct SolidSyslogBuffer* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_BUFFER);
    }
    else
    {
        instance.buffer = configured;
    }
}

static void InstallSender(struct SolidSyslogSender* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_SENDER);
    }
    else
    {
        instance.sender = configured;
    }
}

static void InstallStore(struct SolidSyslogStore* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_STORE);
    }
    else
    {
        instance.store = configured;
    }
}

static void InstallClock(SolidSyslogClockFunction configured)
{
    if (configured != NULL)
    {
        instance.clock = configured;
    }
}

static void InstallHostname(SolidSyslogStringFunction configured)
{
    if (configured != NULL)
    {
        instance.getHostname = configured;
    }
}

static void InstallAppName(SolidSyslogStringFunction configured)
{
    if (configured != NULL)
    {
        instance.getAppName = configured;
    }
}

static void InstallProcessId(SolidSyslogStringFunction configured)
{
    if (configured != NULL)
    {
        instance.getProcessId = configured;
    }
}

static void InstallStructuredData(struct SolidSyslogStructuredData** configured, size_t count)
{
    instance.sd      = configured;
    instance.sdCount = count;
}

void SolidSyslog_Destroy(void)
{
    instance             = NilInstance;
    nilBufferReportArmed = true;
    nilSenderReportArmed = true;
    instanceInitialised  = false;
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
    DrainBufferIntoStore();
    SendOneFromStore();
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
static inline void DrainBufferIntoStore(void)
{
    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t len = 0;

    while (SolidSyslogBuffer_Read(instance.buffer, buf, sizeof(buf), &len))
    {
        if (!SolidSyslogStore_Write(instance.store, buf, len) && SolidSyslogStore_IsTransient(instance.store))
        {
            SolidSyslogSender_Send(instance.sender, buf, len);
        }
    }
}

static inline void SendOneFromStore(void)
{
    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t len = 0;

    if (SolidSyslogStore_ReadNextUnsent(instance.store, buf, sizeof(buf), &len) && SolidSyslogSender_Send(instance.sender, buf, len))
    {
        SolidSyslogStore_MarkSent(instance.store);
    }
}

void SolidSyslog_Log(const struct SolidSyslogMessage* message)
{
    if (message == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_LOG_NULL_MESSAGE);
    }
    else
    {
        SolidSyslogFormatterStorage  storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_MESSAGE_SIZE)];
        struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(storage, SOLIDSYSLOG_MAX_MESSAGE_SIZE);

        FormatMessage(f, message);
        SolidSyslogBuffer_Write(instance.buffer, SolidSyslogFormatter_AsFormattedBuffer(f), SolidSyslogFormatter_Length(f));
    }
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

/* ============================================================================
 * Nil collaborators
 *
 * Private no-op vtables that occupy every instance slot at file load, after
 * _Destroy, and for any required slot the integrator left NULL in their
 * config. Vtable dispatch never NULL-checks — the nils make every collaborator
 * pointer safe to call.
 *
 * NilBuffer and NilSender report one error via SolidSyslog_Error on first
 * use, then silently consume; _Destroy re-arms the flags. NilStore is silent
 * — its absence is a valid integrator choice (matches SolidSyslogNullStore
 * semantics: drain proceeds, fall-through to direct send).
 *
 * The public SolidSyslogNull* family is for integrator-chosen no-ops with
 * different semantics (e.g. NullBuffer is a direct-send shim); these
 * internal nils are crash-safe defaults only.
 * ============================================================================ */

static bool nilBufferReportArmed = true;
static bool nilSenderReportArmed = true;

static void NilClock(struct SolidSyslogTimestamp* ts)
{
    (void) ts;
}

static void NilStringFunction(struct SolidSyslogFormatter* formatter)
{
    (void) formatter;
}

/* NilBuffer */

static void NilBufferWrite(struct SolidSyslogBuffer* self, const void* data, size_t size)
{
    (void) self;
    (void) data;
    (void) size;
    if (nilBufferReportArmed)
    {
        nilBufferReportArmed = false;
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_NIL_BUFFER_USED);
    }
}

static bool NilBufferRead(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead)
{
    (void) self;
    (void) data;
    (void) maxSize;
    *bytesRead = 0;
    return false;
}

static struct SolidSyslogBuffer NilBuffer = {
    .Write = NilBufferWrite,
    .Read  = NilBufferRead,
};

/* NilSender */

static bool NilSenderSend(struct SolidSyslogSender* self, const void* buffer, size_t size)
{
    (void) self;
    (void) buffer;
    (void) size;
    if (nilSenderReportArmed)
    {
        nilSenderReportArmed = false;
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_NIL_SENDER_USED);
    }
    return true;
}

static void NilSenderDisconnect(struct SolidSyslogSender* self)
{
    (void) self;
}

static struct SolidSyslogSender NilSender = {
    .Send       = NilSenderSend,
    .Disconnect = NilSenderDisconnect,
};

/* NilStore */

static bool NilStoreWrite(struct SolidSyslogStore* self, const void* data, size_t size)
{
    (void) self;
    (void) data;
    (void) size;
    return false;
}

static bool NilStoreReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead)
{
    (void) self;
    (void) data;
    (void) maxSize;
    *bytesRead = 0;
    return false;
}

static void NilStoreMarkSent(struct SolidSyslogStore* self)
{
    (void) self;
}

static bool NilStoreHasUnsent(struct SolidSyslogStore* self)
{
    (void) self;
    return false;
}

static bool NilStoreIsHalted(struct SolidSyslogStore* self)
{
    (void) self;
    return false;
}

static size_t NilStoreGetTotalBytes(struct SolidSyslogStore* self)
{
    (void) self;
    return 0;
}

static size_t NilStoreGetUsedBytes(struct SolidSyslogStore* self)
{
    (void) self;
    return 0;
}

/* NilStore stands in when the integrator passes config.store = NULL —
 * "no store, just try to send." Same transient semantics as NullStore:
 * Service falls through to the sender on Write rejection. */
static bool NilStoreIsTransient(struct SolidSyslogStore* self)
{
    (void) self;
    return true;
}

static struct SolidSyslogStore NilStore = {
    .Write          = NilStoreWrite,
    .ReadNextUnsent = NilStoreReadNextUnsent,
    .MarkSent       = NilStoreMarkSent,
    .HasUnsent      = NilStoreHasUnsent,
    .IsHalted       = NilStoreIsHalted,
    .GetTotalBytes  = NilStoreGetTotalBytes,
    .GetUsedBytes   = NilStoreGetUsedBytes,
    .IsTransient    = NilStoreIsTransient,
};
