#ifndef WOLFSSLTESTRNG_H
#define WOLFSSLTESTRNG_H

#include "SolidSyslogExternC.h"

struct WC_RNG;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* One initialised wolfCrypt RNG for a test, kept behind a C seam because
     * wolfSSL's random.h is not clean under C++ -Wpedantic. Get initialises it
     * on first use; Release frees it. */
    struct WC_RNG* WolfSslTestRng_Get(void);
    void WolfSslTestRng_Release(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* WOLFSSLTESTRNG_H */
