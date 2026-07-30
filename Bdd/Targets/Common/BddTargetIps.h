#ifndef BDDTARGETIPS_H
#define BDDTARGETIPS_H

#include "SolidSyslogExternC.h"

#include <stddef.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogSdValue;

    size_t BddTargetIps_Count(void* context);
    void BddTargetIps_At(struct SolidSyslogSdValue * value, void* context, size_t index);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETIPS_H */
