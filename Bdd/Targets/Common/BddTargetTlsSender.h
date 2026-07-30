#ifndef BDDTARGETTLSSENDER_H
#define BDDTARGETTLSSENDER_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogResolver;
    struct SolidSyslogSender;
    struct mbedtls_ctr_drbg_context;

    struct SolidSyslogSender* BddTargetTlsSender_Create(struct SolidSyslogResolver * resolver, bool mtls);
    void BddTargetTlsSender_Destroy(void);

    /* The seeded CTR-DRBG this module owns, so the at-rest AES-256-GCM policy can
     * reuse it as its nonce source rather than standing up a second entropy /
     * DRBG pair on this resource-constrained demo target. Valid once Create has
     * run (InteractiveTask seeds it during Setup, before any `store file`
     * rebuild). Returns the address of the file-scope context even if seeding
     * failed — the policy's nonce draw then fails closed, which is the correct
     * degraded behaviour. */
    struct mbedtls_ctr_drbg_context* BddTargetTlsSender_GetRng(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETTLSSENDER_H */
