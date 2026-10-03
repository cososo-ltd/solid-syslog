#include "BddTargetMessageSettings.h"

#include <stdbool.h>
#include <string.h>

#include "SolidSyslog.h"
#include "SolidSyslogTunables.h"

static char messageId[33];
static char msg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
static struct SolidSyslogMessage message = {
    .MessageId = messageId,
    .Msg = msg,
};

static inline bool MessageSettings_TryUpdateString(char* storage, size_t storageSize, const char* value);

bool BddTargetMessageSettings_SetByName(const char* name, const char* value)
{
    bool taken = false;
    if (strcmp(name, "msgid") == 0)
    {
        taken = MessageSettings_TryUpdateString(messageId, sizeof(messageId), value);
    }
    else if (strcmp(name, "msg") == 0)
    {
        taken = MessageSettings_TryUpdateString(msg, sizeof(msg), value);
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

const struct SolidSyslogMessage* BddTargetMessageSettings_Message(void)
{
    return &message;
}
