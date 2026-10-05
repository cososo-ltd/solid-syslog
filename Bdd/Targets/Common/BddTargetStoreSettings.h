#ifndef BDDTARGETSTORESETTINGS_H
#define BDDTARGETSTORESETTINGS_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBlockStore.h"
#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The at-rest integrity policies the harness can ask for. A target builds
       only those its platform carries. */
    enum BddTargetSecurityPolicy
    {
        BDD_TARGET_SECURITY_POLICY_CRC16,
        BDD_TARGET_SECURITY_POLICY_NULL,
        BDD_TARGET_SECURITY_POLICY_HMAC_SHA256,
        BDD_TARGET_SECURITY_POLICY_AES_256_GCM
    };

    /* The file store a BDD target builds on `set store file`, shaped by the
       `set` lines the harness sends before it. */
    void BddTargetStoreSettings_Reset(void);
    bool BddTargetStoreSettings_SetByName(const char* name, const char* value);
    size_t BddTargetStoreSettings_MaxBlocks(void);
    size_t BddTargetStoreSettings_MaxBlockSize(void);
    enum SolidSyslogDiscardPolicy BddTargetStoreSettings_DiscardPolicy(void);
    enum BddTargetSecurityPolicy BddTargetStoreSettings_SecurityPolicy(void);
    /* Whether a full store under the halt policy ends the run, with status 2. */
    bool BddTargetStoreSettings_HaltExit(void);
    /* Whether the logger carries the meta SD alone, without timeQuality and origin. */
    bool BddTargetStoreSettings_NoSd(void);
    /* The BlockStore's GetCapacityThreshold callback; the context is unused. */
    size_t BddTargetStoreSettings_GetCapacityThreshold(void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETSTORESETTINGS_H */
