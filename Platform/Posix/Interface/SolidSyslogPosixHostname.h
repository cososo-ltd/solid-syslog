/** @file
 *  The POSIX SolidSyslogHeaderFieldFunction for RFC 5424 HOSTNAME, for
 *  SolidSyslogConfig.GetHostname. */
#ifndef SOLIDSYSLOGPOSIXHOSTNAME_H
#define SOLIDSYSLOGPOSIXHOSTNAME_H

#include "SolidSyslogExternC.h"

struct SolidSyslogHeaderField;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Writes the host's name (gethostname) into @p field. @p context is unused. */
    void SolidSyslogPosix_GetHostname(struct SolidSyslogHeaderField * field, void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXHOSTNAME_H */
