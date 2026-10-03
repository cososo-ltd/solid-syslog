#ifndef BDDTARGETMESSAGESETTINGS_H
#define BDDTARGETMESSAGESETTINGS_H

#include <stdbool.h>

#include "SolidSyslogExternC.h"

struct SolidSyslogMessage;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The message and destination a BDD target logs with, changed by `set`
       lines from the harness. */
    bool BddTargetMessageSettings_SetByName(const char* name, const char* value);
    const struct SolidSyslogMessage* BddTargetMessageSettings_Message(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETMESSAGESETTINGS_H */
