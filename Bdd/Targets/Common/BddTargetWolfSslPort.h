#ifndef BDDTARGETWOLFSSLPORT_H
#define BDDTARGETWOLFSSLPORT_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* What a wolfSSL built from source asks its target for: a seed and a clock.
       The two functions wolfSSL itself calls are named by the target's
       user_settings.h and are not declared here. */
    void BddTargetWolfSslPort_Initialise(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETWOLFSSLPORT_H */
