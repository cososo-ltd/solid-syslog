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
    bool taken = false;
    size_t length = strlen(value);
    if ((strcmp(name, "msgid") == 0) && (length > 0U) && (length < sizeof(messageId)))
    {
        (void) memcpy(messageId, value, length + 1U);
        taken = true;
    }
    return taken;
}

const struct SolidSyslogMessage* BddTargetMessageSettings_Message(void)
{
    return &message;
}
