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

static char appName[49];
static char messageId[33];
static char msg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
static char host[16];
static uint16_t port;
static uint32_t endpointVersion;
static struct SolidSyslogMessage message = {
    .MessageId = messageId,
    .Msg = msg,
};

static inline bool MessageSettings_TryUpdateString(char* storage, size_t storageSize, const char* value);
static inline bool MessageSettings_TryParseNumber(const char* value, unsigned long* parsed);

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
        taken = MessageSettings_TryUpdateString(host, sizeof(host), value);
        if (taken)
        {
            endpointVersion++;
        }
    }
    else if (strcmp(name, "port") == 0)
    {
        unsigned long parsed = 0U;
        taken = MessageSettings_TryParseNumber(value, &parsed) && (parsed > 0U) && (parsed <= UINT16_MAX);
        if (taken)
        {
            port = (uint16_t) parsed;
            endpointVersion++;
        }
    }
    else if (strcmp(name, "facility") == 0)
    {
        unsigned long parsed = 0U;
        taken = MessageSettings_TryParseNumber(value, &parsed);
        if (taken)
        {
            message.Facility = (enum SolidSyslogFacility) parsed;
        }
    }
    else if (strcmp(name, "severity") == 0)
    {
        unsigned long parsed = 0U;
        taken = MessageSettings_TryParseNumber(value, &parsed);
        if (taken)
        {
            message.Severity = (enum SolidSyslogSeverity) parsed;
        }
    }
    else
    {
        /* Not ours - taken stays false so the caller can offer it elsewhere. */
    }
    return taken;
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

/* Takes decimal digits only, with nothing after them. A value outside the
   enumeration is passed on unchanged, so the library stays the one authority
   on what is valid. */
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
