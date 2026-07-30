/** @file
 *  The no-op SecurityPolicy Null object: pass-through integrity — Seal and Open both return
 *  true without touching the record, adding no integrity data and accepting every record. */
#ifndef SOLIDSYSLOGNULLSECURITYPOLICY_H
#define SOLIDSYSLOGNULLSECURITYPOLICY_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Pass-through: Seal and Open both return true without touching the record, adding
     *  no integrity data and accepting every record as valid. */
    struct SolidSyslogSecurityPolicy* SolidSyslogNullSecurityPolicy_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLSECURITYPOLICY_H */
