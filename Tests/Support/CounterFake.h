#ifndef COUNTERFAKE_H
#define COUNTERFAKE_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAtomicCounter;

    /* A single-task AtomicCounter counting from 1, for a sequenceId source
     * that needs no platform: tests that read the numbers back, not the
     * atomicity. */
    struct SolidSyslogAtomicCounter* CounterFake_Create(void);
    void CounterFake_Destroy(struct SolidSyslogAtomicCounter * counter);
    /* The number the last Increment handed out; zero before the first. */
    uint32_t CounterFake_LastHandedOut(struct SolidSyslogAtomicCounter * counter);

SOLIDSYSLOG_EXTERN_C_END

#endif /* COUNTERFAKE_H */
