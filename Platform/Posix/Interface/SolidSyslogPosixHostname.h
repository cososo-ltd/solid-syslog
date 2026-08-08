/** @file
 *  The POSIX SolidSyslogHeaderFieldFunction for RFC 5424 HOSTNAME, for
 *  SolidSyslogConfig.GetHostname.
 *
 *  @ingroup platform_posix */
#ifndef SOLIDSYSLOGPOSIXHOSTNAME_H
#define SOLIDSYSLOGPOSIXHOSTNAME_H

#include "SolidSyslogExternC.h"

struct SolidSyslogHeaderField;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Writes the host's name (gethostname) into @p field. @p context is unused. */
    void SolidSyslogPosixHostname_Get(struct SolidSyslogHeaderField * field, void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXHOSTNAME_H */
