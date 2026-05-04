#ifndef SOLIDSYSLOGCRC16POLICY_H
#define SOLIDSYSLOGCRC16POLICY_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogSecurityPolicy* SolidSyslogCrc16Policy_Create(void);
    void                              SolidSyslogCrc16Policy_Destroy(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGCRC16POLICY_H */
