#ifndef BDDTARGETAPPNAME_H
#define BDDTARGETAPPNAME_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogHeaderField;

    void BddTargetAppName_Set(const char* argv0);
    void BddTargetAppName_Get(struct SolidSyslogHeaderField * field, void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETAPPNAME_H */
