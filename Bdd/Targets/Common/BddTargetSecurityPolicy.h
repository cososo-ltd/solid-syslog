#ifndef BDDTARGETSECURITYPOLICY_H
#define BDDTARGETSECURITYPOLICY_H

#include "SolidSyslogExternC.h"

struct SolidSyslogSecurityPolicy;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The at-rest policy `set security-policy` named, built from whichever
       crypto pack the target was linked with. One translation unit per pack
       fills this seam; the host CMake picks the file, so the choice is a link
       decision rather than a conditional in the target. */
    struct SolidSyslogSecurityPolicy* BddTargetSecurityPolicy_Create(void);
    /* Destroys what Create built; the Null policy and CRC-16 need no pack. */
    void BddTargetSecurityPolicy_Destroy(struct SolidSyslogSecurityPolicy* policy);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETSECURITYPOLICY_H */
