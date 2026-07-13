/** @file
 *  The Windows SolidSyslogHeaderFieldFunction for RFC 5424 HOSTNAME, for
 *  SolidSyslogConfig.GetHostname. */
#ifndef SOLIDSYSLOGWINDOWSHOSTNAME_H
#define SOLIDSYSLOGWINDOWSHOSTNAME_H

#include "SolidSyslogConfig.h"

EXTERN_C_BEGIN

    /** Writes the physical DNS host name (GetComputerNameExA) into @p field.
     *  @p context is unused. */
    void SolidSyslogWindowsHostname_Get(struct SolidSyslogHeaderField * field, void* context);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSHOSTNAME_H */
