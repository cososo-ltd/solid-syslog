#ifndef BDDTARGETSWITCHCONFIG_H
#define BDDTARGETSWITCHCONFIG_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

EXTERN_C_BEGIN

    enum
    {
        BDD_TARGET_SWITCH_UDP,
        BDD_TARGET_SWITCH_TCP,
        BDD_TARGET_SWITCH_TLS,
        BDD_TARGET_SWITCH_COUNT,
    };

    void BddTargetSwitchConfig_SetByName(const char* name);
    uint8_t BddTargetSwitchConfig_Selector(void);
    /* tls and mtls share the same Switching slot (BDD_TARGET_SWITCH_TLS) so
     * the TLS sender / TLS stream / underlying TCP socket get reused across
     * the two modes. The FreeRTOS BDD target reads this at TLS Connect time
     * to pick between the syslog-ng oracle's port 6514 (TLS) and 6515 (mTLS);
     * the client cert is wired regardless of mode, so the port is the only
     * thing that has to flip. */
    bool BddTargetSwitchConfig_IsMtlsMode(void);

EXTERN_C_END

#endif /* BDDTARGETSWITCHCONFIG_H */
