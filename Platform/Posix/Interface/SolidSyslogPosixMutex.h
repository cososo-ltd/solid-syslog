/** @file
 *  A Mutex wrapping pthread_mutex_t, for thread-safe buffers and pools on a
 *  POSIX host.
 *
 *  @ingroup platform_posix */
#ifndef SOLIDSYSLOGPOSIXMUTEX_H
#define SOLIDSYSLOGPOSIXMUTEX_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    /** Create takes no config; an exhausted pool — or a failed pthread_mutex_init
     *  — falls back to the shared NullMutex, whose Lock and Unlock are no-ops. */
    struct SolidSyslogMutex* SolidSyslogPosixMutex_Create(void);
    /** Release the pool slot; destroys the underlying pthread_mutex_t (only if it
     *  was successfully initialised). */
    void SolidSyslogPosixMutex_Destroy(struct SolidSyslogMutex * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMUTEX_H */
