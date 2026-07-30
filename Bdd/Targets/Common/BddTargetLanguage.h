#ifndef BDDTARGETLANGUAGE_H
#define BDDTARGETLANGUAGE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogSdValue;

    void BddTargetLanguage_Get(struct SolidSyslogSdValue * value, void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETLANGUAGE_H */
