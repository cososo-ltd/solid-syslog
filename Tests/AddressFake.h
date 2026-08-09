#ifndef ADDRESSFAKE_H
#define ADDRESSFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /*
     * Returns an opaque SolidSyslogAddress* suitable for tests that treat
     * Address as a pure pass-through token (StreamFakeTest, OpenSslStreamTest)
     * — they never read the platform sockaddr inside. Platform-agnostic so
     * the same test executable builds on POSIX, Winsock, and FreeRTOS hosts
     * without conditional compilation.
     */
    struct SolidSyslogAddress* AddressFake_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* ADDRESSFAKE_H */
