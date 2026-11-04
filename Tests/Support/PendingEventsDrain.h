#ifndef PENDINGEVENTSDRAIN_H
#define PENDINGEVENTSDRAIN_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The pending-event table is static and has no production reset: a
     * pending kind is meant to survive until Service takes it. A test that
     * raised one would otherwise hand it to the next test that runs Service,
     * so test code starts clean by taking every slot. */
    void PendingEventsDrain_All(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* PENDINGEVENTSDRAIN_H */
