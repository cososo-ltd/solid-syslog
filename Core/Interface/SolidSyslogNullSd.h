/** @file
 *  The no-op Structured Data Null object: Format writes nothing, so this SD slot
 *  contributes no SD-ELEMENT to the message. */
#ifndef SOLIDSYSLOGNULLSD_H
#define SOLIDSYSLOGNULLSD_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Format writes nothing, so this SD slot contributes no SD-ELEMENT to the message. */
    struct SolidSyslogStructuredData* SolidSyslogNullSd_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLSD_H */
