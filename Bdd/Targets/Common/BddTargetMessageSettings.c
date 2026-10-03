#include "BddTargetMessageSettings.h"

#include <stdbool.h>
#include <string.h>

#include "SolidSyslog.h"

static char messageId[33];
static struct SolidSyslogMessage message = {
    .MessageId = messageId,
};

bool BddTargetMessageSettings_SetByName(const char* name, const char* value)
{
    (void) name;
    (void) memcpy(messageId, value, strlen(value) + 1U);
    return true;
}

const struct SolidSyslogMessage* BddTargetMessageSettings_Message(void)
{
    return &message;
}
