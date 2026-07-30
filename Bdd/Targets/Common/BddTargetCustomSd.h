#ifndef BDDTARGETCUSTOMSD_H
#define BDDTARGETCUSTOMSD_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogStructuredData;

    /* The worked custom SD-ELEMENT for the integrator guide
     * (docs/structured-data.md). Emits [example@32473 detail="Hello World"].
     * Stateless singleton — handed to SolidSyslog_LogWithSd by the
     * `send-custom` interactive command. */
    struct SolidSyslogStructuredData* BddTargetCustomSd_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETCUSTOMSD_H */
