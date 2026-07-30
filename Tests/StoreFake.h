#ifndef STOREFAKE_H
#define STOREFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogStore* StoreFake_Create(void);
    void StoreFake_Destroy(void);
    void StoreFake_FailNextWrite(void);
    void StoreFake_FailNextRead(void);
    void StoreFake_SetHalted(void);
    int StoreFake_WriteCallCount(struct SolidSyslogStore * store);

SOLIDSYSLOG_EXTERN_C_END

#endif /* STOREFAKE_H */
