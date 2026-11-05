#ifndef STOREFAKE_H
#define STOREFAKE_H

#include <stdbool.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogStore* StoreFake_Create(void);
    void StoreFake_Destroy(void);
    void StoreFake_FailNextWrite(void);
    /* The next write is lost: the fake reports nothing, as a real store would have. */
    void StoreFake_LoseNextWrite(void);
    void StoreFake_FailNextRead(void);
    /* While transient the fake keeps nothing and says so, as a store that has
     * stopped retaining does. */
    void StoreFake_SetTransient(bool transient);
    int StoreFake_WriteCallCount(struct SolidSyslogStore * store);

SOLIDSYSLOG_EXTERN_C_END

#endif /* STOREFAKE_H */
