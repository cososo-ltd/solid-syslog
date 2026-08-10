/** @file
 *  The POSIX SolidSyslogHeaderFieldFunction for RFC 5424 PROCID, for
 *  SolidSyslogConfig.GetProcessId. */
#ifndef SOLIDSYSLOGPOSIXPROCESSID_H
#define SOLIDSYSLOGPOSIXPROCESSID_H

#include "SolidSyslogExternC.h"

struct SolidSyslogHeaderField;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Writes the process id (getpid) into @p field. @p context is unused. */
    void SolidSyslogPosix_GetProcessId(struct SolidSyslogHeaderField * field, void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXPROCESSID_H */
