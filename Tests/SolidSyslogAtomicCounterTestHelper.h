#ifndef SOLIDSYSLOGATOMICCOUNTERTESTHELPER_H
#define SOLIDSYSLOGATOMICCOUNTERTESTHELPER_H

#include "SolidSyslogExternC.h"

#include <stddef.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAtomicCounter;

    struct SolidSyslogAtomicCounter* TestAtomicCounter_Create(void);
    void TestAtomicCounter_Init(struct SolidSyslogAtomicCounter * base, uint32_t value);
    uint32_t TestAtomicCounter_Increment(struct SolidSyslogAtomicCounter * base);
    void TestAtomicCounter_Destroy(struct SolidSyslogAtomicCounter * base);

    /* Pool size of the active backend. Lets backend-agnostic contract tests
       runtime-gate scenarios that need two simultaneous handles. */
    size_t TestAtomicCounter_PoolSize(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGATOMICCOUNTERTESTHELPER_H */
