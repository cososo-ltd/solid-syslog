/** @file
 *  The Windows SolidSyslogHeaderFieldFunction for RFC 5424 PROCID, for
 *  SolidSyslogConfig.GetProcessId. */
#ifndef SOLIDSYSLOGWINDOWSPROCESSID_H
#define SOLIDSYSLOGWINDOWSPROCESSID_H

#include "SolidSyslogConfig.h"

EXTERN_C_BEGIN

    /** Writes the process id (GetCurrentProcessId) into @p field. @p context is
     *  unused. */
    void SolidSyslogWindowsProcessId_Get(struct SolidSyslogHeaderField * field, void* context);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSPROCESSID_H */
