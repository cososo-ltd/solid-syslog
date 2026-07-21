#include "BddTargetSwitchConfig.h"

#include <string.h>

static volatile uint8_t selectedIndex;
static volatile bool mtlsMode;

void BddTargetSwitchConfig_SetByName(const char* name)
{
    if (strcmp(name, "udp") == 0)
    {
        selectedIndex = BDD_TARGET_SWITCH_UDP;
        mtlsMode = false;
    }
    else if (strcmp(name, "tcp") == 0)
    {
        selectedIndex = BDD_TARGET_SWITCH_TCP;
        mtlsMode = false;
    }
    else if (strcmp(name, "tls") == 0)
    {
        selectedIndex = BDD_TARGET_SWITCH_TLS;
        mtlsMode = false;
    }
    else if (strcmp(name, "mtls") == 0)
    {
        /* mTLS shares the reliable/TLS slot. The wrapper wires the client
         * cert+key at startup regardless of mode, and the TLS sender's
         * Endpoint callback dispatches on mtlsMode to pick the right port
         * (6514 vs 6515) at Connect time. */
        selectedIndex = BDD_TARGET_SWITCH_TLS;
        mtlsMode = true;
    }
}

uint8_t BddTargetSwitchConfig_Selector(void* context)
{
    (void) context;
    return selectedIndex;
}

bool BddTargetSwitchConfig_IsMtlsMode(void)
{
    return mtlsMode;
}
