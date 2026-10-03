#include "BddTargetMessageSettings.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "SolidSyslog.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogEndpointHost.h"
#include "SolidSyslogHeaderField.h"
#include "SolidSyslogTunables.h"

/* Storage holds RFC 5424's maxima where it sets one (APP-NAME 48, MSGID 32)
   plus the terminator; MSG matches SOLIDSYSLOG_MAX_MESSAGE_SIZE; the host fits
   an IPv4 dotted quad or a short name. */
enum
{
    APP_NAME_SIZE = 49,
    MESSAGE_ID_SIZE = 33,
    HOST_SIZE = 16,
    DEFAULT_PORT = 5514
};

static const char DEFAULT_APP_NAME[] = "SolidSyslogBddTarget";
static const char DEFAULT_MESSAGE_ID[] = "example";
static const char DEFAULT_MSG[] = "Hello from SolidSyslog";

static char appName[APP_NAME_SIZE];
static char messageId[MESSAGE_ID_SIZE];
static char msg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
static char host[HOST_SIZE];
static uint16_t port;
static uint32_t endpointVersion;
static struct SolidSyslogMessage message = {
    .MessageId = messageId,
    .Msg = msg,
};

static inline bool MessageSettings_SetHost(const char* value);
static inline bool MessageSettings_SetPort(const char* value);
static inline bool MessageSettings_IsPort(unsigned long number);
static inline bool MessageSettings_SetFacility(const char* value);
static inline bool MessageSettings_SetSeverity(const char* value);
static inline bool MessageSettings_TryUpdateString(char* storage, size_t storageSize, const char* value);
static inline bool MessageSettings_TryParseNumber(const char* value, unsigned long* parsed);

void BddTargetMessageSettings_Reset(const char* defaultHost)
{
    (void) MessageSettings_TryUpdateString(host, sizeof(host), defaultHost);
    port = (uint16_t) DEFAULT_PORT;
    endpointVersion++;
    (void) MessageSettings_TryUpdateString(appName, sizeof(appName), DEFAULT_APP_NAME);
    (void) MessageSettings_TryUpdateString(messageId, sizeof(messageId), DEFAULT_MESSAGE_ID);
    (void) MessageSettings_TryUpdateString(msg, sizeof(msg), DEFAULT_MSG);
    message.Facility = SOLIDSYSLOG_FACILITY_LOCAL0;
    message.Severity = SOLIDSYSLOG_SEVERITY_INFORMATIONAL;
}

/* Takes a value that is neither empty nor too long for its storage, and leaves
   the storage alone otherwise. */
static inline bool MessageSettings_TryUpdateString(char* storage, size_t storageSize, const char* value)
{
    size_t length = strlen(value);
    bool fits = (length > 0U) && (length < storageSize);
    if (fits)
    {
        (void) memcpy(storage, value, length + 1U);
    }
    return fits;
}

bool BddTargetMessageSettings_SetByName(const char* name, const char* value)
{
    bool taken = false;
    if (strcmp(name, "appname") == 0)
    {
        taken = MessageSettings_TryUpdateString(appName, sizeof(appName), value);
    }
    else if (strcmp(name, "msgid") == 0)
    {
        taken = MessageSettings_TryUpdateString(messageId, sizeof(messageId), value);
    }
    else if (strcmp(name, "msg") == 0)
    {
        taken = MessageSettings_TryUpdateString(msg, sizeof(msg), value);
    }
    else if (strcmp(name, "host") == 0)
    {
        taken = MessageSettings_SetHost(value);
    }
    else if (strcmp(name, "port") == 0)
    {
        taken = MessageSettings_SetPort(value);
    }
    else if (strcmp(name, "facility") == 0)
    {
        taken = MessageSettings_SetFacility(value);
    }
    else if (strcmp(name, "severity") == 0)
    {
        taken = MessageSettings_SetSeverity(value);
    }
    else
    {
        /* Not ours - taken stays false so the caller can offer it elsewhere. */
    }
    return taken;
}

/* A new destination moves the endpoint version, so the sender resolves again. */
static inline bool MessageSettings_SetHost(const char* value)
{
    bool taken = MessageSettings_TryUpdateString(host, sizeof(host), value);
    if (taken)
    {
        endpointVersion++;
    }
    return taken;
}

static inline bool MessageSettings_SetPort(const char* value)
{
    unsigned long parsed = 0U;
    bool taken = MessageSettings_TryParseNumber(value, &parsed) && MessageSettings_IsPort(parsed);
    if (taken)
    {
        port = (uint16_t) parsed;
        endpointVersion++;
    }
    return taken;
}

/* Takes decimal digits only, with nothing after them. */
static inline bool MessageSettings_TryParseNumber(const char* value, unsigned long* parsed)
{
    char* end = NULL;
    unsigned long number = strtoul(value, &end, 10);
    bool isNumber = (end != value) && (*end == '\0');
    if (isNumber)
    {
        *parsed = number;
    }
    return isNumber;
}

static inline bool MessageSettings_IsPort(unsigned long number)
{
    return (number > 0U) && (number <= UINT16_MAX);
}

/* A value outside the enumeration is passed on unchanged, so the library stays
   the one authority on what is valid. */
static inline bool MessageSettings_SetFacility(const char* value)
{
    unsigned long parsed = 0U;
    bool taken = MessageSettings_TryParseNumber(value, &parsed);
    if (taken)
    {
        message.Facility = (enum SolidSyslogFacility) parsed;
    }
    return taken;
}

static inline bool MessageSettings_SetSeverity(const char* value)
{
    unsigned long parsed = 0U;
    bool taken = MessageSettings_TryParseNumber(value, &parsed);
    if (taken)
    {
        message.Severity = (enum SolidSyslogSeverity) parsed;
    }
    return taken;
}

const struct SolidSyslogMessage* BddTargetMessageSettings_Message(void)
{
    return &message;
}

void BddTargetMessageSettings_GetAppName(struct SolidSyslogHeaderField* field, void* context)
{
    (void) context;
    SolidSyslogHeaderField_PrintUsAscii(field, appName, strlen(appName));
}

void BddTargetMessageSettings_GetEndpoint(struct SolidSyslogEndpoint* endpoint, void* context)
{
    (void) context;
    SolidSyslogEndpointHost_String(endpoint->Host, host, strlen(host));
    endpoint->Port = port;
}

uint32_t BddTargetMessageSettings_GetEndpointVersion(void* context)
{
    (void) context;
    return endpointVersion;
}
