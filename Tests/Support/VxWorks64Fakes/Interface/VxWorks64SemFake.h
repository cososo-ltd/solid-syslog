#ifndef VXWORKS64SEMFAKE_H
#define VXWORKS64SEMFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void VxWorks64SemFake_Reset(void);

    unsigned VxWorks64SemFake_SemMCreateCallCount(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64SEMFAKE_H */
