/** @file
 *  A Mutex wrapping a Windows CRITICAL_SECTION, for thread-safe buffers and
 *  pools on a Windows host. */
#ifndef SOLIDSYSLOGWINDOWSMUTEX_H
#define SOLIDSYSLOGWINDOWSMUTEX_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    /** Create takes no config and initialises the CRITICAL_SECTION; an exhausted
     *  pool falls back to the shared NullMutex, whose Lock and Unlock are no-ops. */
    struct SolidSyslogMutex* SolidSyslogWindowsMutex_Create(void);
    /** Release the pool slot; deletes the underlying CRITICAL_SECTION. */
    void SolidSyslogWindowsMutex_Destroy(struct SolidSyslogMutex * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSMUTEX_H */
