#ifndef BDDTARGETIPS_H
#define BDDTARGETIPS_H

#include "ExternC.h"

#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslogSdValue;

    size_t BddTargetIps_Count(void* context);
    void BddTargetIps_At(struct SolidSyslogSdValue * value, void* context, size_t index);

EXTERN_C_END

#endif /* BDDTARGETIPS_H */
