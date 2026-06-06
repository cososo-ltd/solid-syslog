#include "SolidSyslogMessageFormatter.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslog.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSdElementPrivate.h"
#include "SolidSyslogStringFunction.h"
#include "SolidSyslogStructuredData.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogTimestampFormatter.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogFormatter;

enum
{
    SOLIDSYSLOG_MAX_APP_NAME_SIZE = 49,
    SOLIDSYSLOG_MAX_HOSTNAME_SIZE = 256,
    SOLIDSYSLOG_MAX_MSGID_SIZE = 33,
    SOLIDSYSLOG_MAX_PROCESS_ID_SIZE = 129
};

static inline void MessageFormatter_FormatPrival(struct SolidSyslogFormatter* f, uint8_t prival);
static inline uint8_t MessageFormatter_MakePrival(const struct SolidSyslogMessage* message);
static inline uint8_t MessageFormatter_CombineFacilityAndSeverity(uint8_t facility, uint8_t severity);
static inline bool MessageFormatter_PrivalComponentsAreValid(uint8_t facility, uint8_t severity);
static inline bool MessageFormatter_FacilityIsValid(uint8_t facility);
static inline bool MessageFormatter_SeverityIsValid(uint8_t severity);
static inline void MessageFormatter_FormatTimestamp(struct SolidSyslogFormatter* f, SolidSyslogClockFunction clock);
static inline void MessageFormatter_FormatStringField(
    struct SolidSyslogFormatter* f,
    SolidSyslogStringFunction fn,
    size_t maxSize
);
static inline void MessageFormatter_FormatMsgId(struct SolidSyslogFormatter* f, const char* messageId);
static inline bool MessageFormatter_StringIsValid(const char* value);
static inline void MessageFormatter_FormatStructuredData(
    struct SolidSyslogFormatter* f,
    struct SolidSyslogStructuredData** sd,
    size_t sdCount
);
static inline void MessageFormatter_FormatMsg(struct SolidSyslogFormatter* f, const char* msg);
static inline const char* MessageFormatter_SkipLeadingBom(const char* msg);

void SolidSyslogMessageFormatter_Format(
    struct SolidSyslogFormatter* f,
    const struct SolidSyslogMessage* message,
    const struct SolidSyslogMessageFormatterContext* context
)
{
    MessageFormatter_FormatPrival(f, MessageFormatter_MakePrival(message));
    SolidSyslogFormatter_AsciiCharacter(f, '1');
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    MessageFormatter_FormatTimestamp(f, context->Clock);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    MessageFormatter_FormatStringField(f, context->GetHostname, SOLIDSYSLOG_MAX_HOSTNAME_SIZE);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    MessageFormatter_FormatStringField(f, context->GetAppName, SOLIDSYSLOG_MAX_APP_NAME_SIZE);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    MessageFormatter_FormatStringField(f, context->GetProcessId, SOLIDSYSLOG_MAX_PROCESS_ID_SIZE);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    MessageFormatter_FormatMsgId(f, message->MessageId);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    MessageFormatter_FormatStructuredData(f, context->Sd, context->SdCount);
    MessageFormatter_FormatMsg(f, message->Msg);
}

static inline void MessageFormatter_FormatPrival(struct SolidSyslogFormatter* f, uint8_t prival)
{
    SolidSyslogFormatter_AsciiCharacter(f, '<');
    SolidSyslogFormatter_Uint32(f, prival);
    SolidSyslogFormatter_AsciiCharacter(f, '>');
}

static inline uint8_t MessageFormatter_MakePrival(const struct SolidSyslogMessage* message)
{
    uint8_t f = (uint8_t) message->Facility;
    uint8_t s = (uint8_t) message->Severity;
    uint8_t prival =
        MessageFormatter_CombineFacilityAndSeverity(SOLIDSYSLOG_FACILITY_SYSLOG, SOLIDSYSLOG_SEVERITY_ERROR);

    if (MessageFormatter_PrivalComponentsAreValid(f, s))
    {
        prival = MessageFormatter_CombineFacilityAndSeverity(f, s);
    }

    return prival;
}

static inline uint8_t MessageFormatter_CombineFacilityAndSeverity(uint8_t facility, uint8_t severity)
{
    return (uint8_t) ((facility * UINT8_C(8)) + severity);
}

static inline bool MessageFormatter_PrivalComponentsAreValid(uint8_t facility, uint8_t severity)
{
    return MessageFormatter_FacilityIsValid(facility) && MessageFormatter_SeverityIsValid(severity);
}

static inline bool MessageFormatter_FacilityIsValid(uint8_t facility)
{
    return facility <= (uint8_t) SOLIDSYSLOG_FACILITY_LOCAL7;
}

static inline bool MessageFormatter_SeverityIsValid(uint8_t severity)
{
    return severity <= (uint8_t) SOLIDSYSLOG_SEVERITY_DEBUG;
}

static inline void MessageFormatter_FormatTimestamp(struct SolidSyslogFormatter* f, SolidSyslogClockFunction clock)
{
    struct SolidSyslogTimestamp ts = {0};

    clock(&ts);
    SolidSyslogTimestampFormatter_Format(f, &ts);
}

static inline void MessageFormatter_FormatStringField(
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
        SolidSyslogFormatter_NilValue(f);
    }
}

static inline void MessageFormatter_FormatMsgId(struct SolidSyslogFormatter* f, const char* messageId)
{
    size_t lengthBefore = SolidSyslogFormatter_Length(f);

    if (MessageFormatter_StringIsValid(messageId))
    {
        SolidSyslogFormatter_PrintUsAsciiString(f, messageId, SOLIDSYSLOG_MAX_MSGID_SIZE - 1);
    }

    if (SolidSyslogFormatter_Length(f) == lengthBefore)
    {
        SolidSyslogFormatter_NilValue(f);
    }
}

static inline bool MessageFormatter_StringIsValid(const char* value)
{
    return (value != NULL) && (value[0] != '\0');
}

static inline void MessageFormatter_FormatStructuredData(
    struct SolidSyslogFormatter* f,
    struct SolidSyslogStructuredData** sd,
    size_t sdCount
)
{
    size_t lengthBefore = SolidSyslogFormatter_Length(f);
    struct SolidSyslogSdElement element;

    SolidSyslogSdElement_FromFormatter(&element, f);
    for (size_t i = 0; i < sdCount; i++)
    {
        SolidSyslogStructuredData_Format(sd[i], &element);
    }

    if (SolidSyslogFormatter_Length(f) == lengthBefore)
    {
        SolidSyslogFormatter_NilValue(f);
    }
}

static inline void MessageFormatter_FormatMsg(struct SolidSyslogFormatter* f, const char* msg)
{
    /* Guard msg before SkipLeadingBom dereferences it, then guard the
     * post-strip body so a caller-supplied BOM-only string emits no
     * dangling SP-BOM (RFC 5424 §6.4 — the BOM belongs to a non-empty MSG). */
    if (MessageFormatter_StringIsValid(msg))
    {
        const char* body = MessageFormatter_SkipLeadingBom(msg);

        if (MessageFormatter_StringIsValid(body))
        {
            SolidSyslogFormatter_AsciiCharacter(f, ' ');
            SolidSyslogFormatter_Bom(f);
            SolidSyslogFormatter_BoundedString(f, body, SOLIDSYSLOG_MAX_MESSAGE_SIZE);
        }
    }
}

/* MSG body MUST start with the UTF-8 BOM per RFC 5424 §6.4. We always
 * emit the BOM ourselves; if the caller already prefixed one, strip it
 * so the wire frame contains exactly one. */
static inline const char* MessageFormatter_SkipLeadingBom(const char* msg)
{
    const unsigned char* bytes = (const unsigned char*) msg;
    const char* result = msg;
    if ((bytes[0] == 0xEFU) && (bytes[1] == 0xBBU) && (bytes[2] == 0xBFU))
    {
        result = &msg[3];
    }
    return result;
}
