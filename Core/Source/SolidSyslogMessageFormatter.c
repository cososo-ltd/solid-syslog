/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogMessageFormatter.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslog.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogHeaderFieldFunction.h"
#include "SolidSyslogHeaderFieldPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSdElementPrivate.h"
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

static inline void MessageFormatter_FormatPrival(struct SolidSyslogFormatter* formatter, uint8_t prival);
static inline uint8_t MessageFormatter_MakePrival(const struct SolidSyslogMessage* message);
static inline uint8_t MessageFormatter_CombineFacilityAndSeverity(uint8_t facility, uint8_t severity);
static inline bool MessageFormatter_PrivalComponentsAreValid(uint8_t facility, uint8_t severity);
static inline bool MessageFormatter_FacilityIsValid(uint8_t facility);
static inline bool MessageFormatter_SeverityIsValid(uint8_t severity);
static inline void MessageFormatter_FormatTimestamp(
    struct SolidSyslogFormatter* formatter,
    SolidSyslogClockFunction clock
);
static inline void MessageFormatter_FormatStringField(
    struct SolidSyslogFormatter* formatter,
    SolidSyslogHeaderFieldFunction fn,
    void* context,
    size_t maxSize
);
static inline void MessageFormatter_FormatMsgId(struct SolidSyslogFormatter* formatter, const char* messageId);
static inline bool MessageFormatter_StringIsValid(const char* value);
static inline void MessageFormatter_FormatStructuredData(
    struct SolidSyslogFormatter* formatter,
    struct SolidSyslogStructuredData** baseSd,
    size_t baseSdCount,
    struct SolidSyslogStructuredData** messageSd,
    size_t messageSdCount
);
static inline void MessageFormatter_FormatSdElements(
    struct SolidSyslogSdElement* element,
    struct SolidSyslogStructuredData** sd,
    size_t sdCount
);
static inline void MessageFormatter_FormatMsg(struct SolidSyslogFormatter* formatter, const char* msg);
static inline const char* MessageFormatter_SkipLeadingBom(const char* msg);

void SolidSyslogMessageFormatter_Format(
    struct SolidSyslogFormatter* formatter,
    const struct SolidSyslogMessage* message,
    const struct SolidSyslogMessageFormatterContext* context,
    struct SolidSyslogStructuredData** messageSd,
    size_t messageSdCount
)
{
    MessageFormatter_FormatPrival(formatter, MessageFormatter_MakePrival(message));
    SolidSyslogFormatter_AsciiCharacter(formatter, '1');
    SolidSyslogFormatter_AsciiCharacter(formatter, ' ');
    MessageFormatter_FormatTimestamp(formatter, context->Clock);
    SolidSyslogFormatter_AsciiCharacter(formatter, ' ');
    MessageFormatter_FormatStringField(
        formatter,
        context->GetHostname,
        context->GetHostnameContext,
        SOLIDSYSLOG_MAX_HOSTNAME_SIZE
    );
    SolidSyslogFormatter_AsciiCharacter(formatter, ' ');
    MessageFormatter_FormatStringField(
        formatter,
        context->GetAppName,
        context->GetAppNameContext,
        SOLIDSYSLOG_MAX_APP_NAME_SIZE
    );
    SolidSyslogFormatter_AsciiCharacter(formatter, ' ');
    MessageFormatter_FormatStringField(
        formatter,
        context->GetProcessId,
        context->GetProcessIdContext,
        SOLIDSYSLOG_MAX_PROCESS_ID_SIZE
    );
    SolidSyslogFormatter_AsciiCharacter(formatter, ' ');
    MessageFormatter_FormatMsgId(formatter, message->MessageId);
    SolidSyslogFormatter_AsciiCharacter(formatter, ' ');
    MessageFormatter_FormatStructuredData(formatter, context->Sd, context->SdCount, messageSd, messageSdCount);
    MessageFormatter_FormatMsg(formatter, message->Msg);
}

static inline void MessageFormatter_FormatPrival(struct SolidSyslogFormatter* formatter, uint8_t prival)
{
    SolidSyslogFormatter_AsciiCharacter(formatter, '<');
    SolidSyslogFormatter_Uint32(formatter, prival);
    SolidSyslogFormatter_AsciiCharacter(formatter, '>');
}

static inline uint8_t MessageFormatter_MakePrival(const struct SolidSyslogMessage* message)
{
    uint8_t facility = (uint8_t) message->Facility;
    uint8_t severity = (uint8_t) message->Severity;
    uint8_t prival =
        MessageFormatter_CombineFacilityAndSeverity(SOLIDSYSLOG_FACILITY_SYSLOG, SOLIDSYSLOG_SEVERITY_ERROR);

    if (MessageFormatter_PrivalComponentsAreValid(facility, severity))
    {
        prival = MessageFormatter_CombineFacilityAndSeverity(facility, severity);
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

static inline void MessageFormatter_FormatTimestamp(
    struct SolidSyslogFormatter* formatter,
    SolidSyslogClockFunction clock
)
{
    struct SolidSyslogTimestamp ts = {0};

    clock(&ts);
    SolidSyslogTimestampFormatter_Format(formatter, &ts);
}

static inline void MessageFormatter_FormatStringField(
    struct SolidSyslogFormatter* formatter,
    SolidSyslogHeaderFieldFunction fn,
    void* context,
    size_t maxSize
)
{
    size_t lengthBefore = SolidSyslogFormatter_Length(formatter);
    struct SolidSyslogHeaderField field;

    /* maxSize is the field's storage size (carries a NUL slot); the usable
     * field width is one less - matching the RFC HOSTNAME / APP-NAME / PROCID
     * caps the scratch-field formatter enforced before this writer existed. */
    SolidSyslogHeaderField_FromFormatter(&field, formatter, maxSize - 1U);
    fn(&field, context);

    if (SolidSyslogFormatter_Length(formatter) == lengthBefore)
    {
        SolidSyslogFormatter_NilValue(formatter);
    }
}

static inline void MessageFormatter_FormatMsgId(struct SolidSyslogFormatter* formatter, const char* messageId)
{
    size_t lengthBefore = SolidSyslogFormatter_Length(formatter);

    if (MessageFormatter_StringIsValid(messageId))
    {
        SolidSyslogFormatter_PrintUsAsciiString(formatter, messageId, SOLIDSYSLOG_MAX_MSGID_SIZE - 1);
    }

    if (SolidSyslogFormatter_Length(formatter) == lengthBefore)
    {
        SolidSyslogFormatter_NilValue(formatter);
    }
}

static inline bool MessageFormatter_StringIsValid(const char* value)
{
    return (value != NULL) && (value[0] != '\0');
}

static inline void MessageFormatter_FormatStructuredData(
    struct SolidSyslogFormatter* formatter,
    struct SolidSyslogStructuredData** baseSd,
    size_t baseSdCount,
    struct SolidSyslogStructuredData** messageSd,
    size_t messageSdCount
)
{
    size_t lengthBefore = SolidSyslogFormatter_Length(formatter);
    struct SolidSyslogSdElement element;

    SolidSyslogSdElement_FromFormatter(&element, formatter);
    MessageFormatter_FormatSdElements(&element, baseSd, baseSdCount);
    MessageFormatter_FormatSdElements(&element, messageSd, messageSdCount);

    if (SolidSyslogFormatter_Length(formatter) == lengthBefore)
    {
        SolidSyslogFormatter_NilValue(formatter);
    }
}

static inline void MessageFormatter_FormatSdElements(
    struct SolidSyslogSdElement* element,
    struct SolidSyslogStructuredData** sd,
    size_t sdCount
)
{
    for (size_t i = 0U; i < sdCount; i++)
    {
        /* Skip NULL entries rather than dereference them. Per-instance slots are
           expected to use SolidSyslogNullSd, but a per-message array is supplied
           at the call site where a conditionally-absent SD is naturally NULL -
           the library must not crash on caller input. */
        if (sd[i] != NULL)
        {
            SolidSyslogStructuredData_Format(sd[i], element);
        }
    }
}

static inline void MessageFormatter_FormatMsg(struct SolidSyslogFormatter* formatter, const char* msg)
{
    /* Guard msg before SkipLeadingBom dereferences it, then guard the
     * post-strip body so a caller-supplied BOM-only string emits no
     * dangling SP-BOM (RFC 5424 §6.4 - the BOM belongs to a non-empty MSG). */
    if (MessageFormatter_StringIsValid(msg))
    {
        const char* body = MessageFormatter_SkipLeadingBom(msg);

        if (MessageFormatter_StringIsValid(body))
        {
            SolidSyslogFormatter_AsciiCharacter(formatter, ' ');
            SolidSyslogFormatter_Bom(formatter);
            SolidSyslogFormatter_BoundedString(formatter, body, SOLIDSYSLOG_MAX_MESSAGE_SIZE);
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
