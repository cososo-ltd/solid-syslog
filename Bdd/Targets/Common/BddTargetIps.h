#ifndef BDDTARGETIPS_H
#define BDDTARGETIPS_H

#include "ExternC.h"

#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslogFormatter;

    size_t BddTargetIps_Count(void);
    void BddTargetIps_At(struct SolidSyslogFormatter * formatter, size_t index);

EXTERN_C_END

#endif /* BDDTARGETIPS_H */
