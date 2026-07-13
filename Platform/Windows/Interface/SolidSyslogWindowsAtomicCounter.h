/** @file
 *  An AtomicCounter over the Win32 Interlocked API, backing the RFC 5424
 *  sequenceId on Windows targets without C11 <stdatomic.h> (legacy MSVC).
 *  Increment runs a lock-free InterlockedCompareExchange CAS loop on a
 *  volatile LONG; the sequence is wrap-aware in [1, 2^31 - 1] and skips zero on
 *  wrap, so a returned value is never 0. */
#ifndef SOLIDSYSLOGWINDOWSATOMICCOUNTER_H
#define SOLIDSYSLOGWINDOWSATOMICCOUNTER_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogAtomicCounter;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullAtomicCounter, whose Increment returns 1 unconditionally. */
    struct SolidSyslogAtomicCounter* SolidSyslogWindowsAtomicCounter_Create(void);
    /** Release the pool slot; the counter's state is discarded. */
    void SolidSyslogWindowsAtomicCounter_Destroy(struct SolidSyslogAtomicCounter * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSATOMICCOUNTER_H */
