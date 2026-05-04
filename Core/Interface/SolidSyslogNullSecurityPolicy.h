#ifndef SOLIDSYSLOGNULLSECURITYPOLICY_H
#define SOLIDSYSLOGNULLSECURITYPOLICY_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogSecurityPolicy* SolidSyslogNullSecurityPolicy_Create(void);
    void                              SolidSyslogNullSecurityPolicy_Destroy(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGNULLSECURITYPOLICY_H */
