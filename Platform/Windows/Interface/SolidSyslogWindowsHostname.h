/** @file
 *  The Windows SolidSyslogHeaderFieldFunction for RFC 5424 HOSTNAME, for
 *  SolidSyslogConfig.GetHostname. */
#ifndef SOLIDSYSLOGWINDOWSHOSTNAME_H
#define SOLIDSYSLOGWINDOWSHOSTNAME_H

#include "SolidSyslogConfig.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Writes the physical DNS host name (GetComputerNameExA) into @p field.
     *  @p context is unused. */
    void SolidSyslogWindows_GetHostname(struct SolidSyslogHeaderField * field, void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSHOSTNAME_H */
