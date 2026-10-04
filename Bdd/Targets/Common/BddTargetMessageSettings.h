#ifndef BDDTARGETMESSAGESETTINGS_H
#define BDDTARGETMESSAGESETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogExternC.h"

struct SolidSyslogEndpoint;
struct SolidSyslogHeaderField;
struct SolidSyslogMessage;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The message and destination a BDD target logs with, changed by `set`
       lines from the harness. */
    void BddTargetMessageSettings_Reset(const char* defaultHost);
    bool BddTargetMessageSettings_SetByName(const char* name, const char* value);
    const struct SolidSyslogMessage* BddTargetMessageSettings_Message(void);
    void BddTargetMessageSettings_GetAppName(struct SolidSyslogHeaderField * field, void* context);
    void BddTargetMessageSettings_GetEndpoint(struct SolidSyslogEndpoint * endpoint, void* context);
    uint32_t BddTargetMessageSettings_GetEndpointVersion(void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETMESSAGESETTINGS_H */
