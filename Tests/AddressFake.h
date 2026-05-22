#ifndef ADDRESSFAKE_H
#define ADDRESSFAKE_H

#include "ExternC.h"

EXTERN_C_BEGIN

    /*
     * Returns an opaque SolidSyslogAddress* suitable for tests that treat
     * Address as a pure pass-through token (StreamFakeTest, TlsStreamTest)
     * — they never read the platform sockaddr inside. Platform-agnostic so
     * the same test executable builds on POSIX, Winsock, and FreeRTOS hosts
     * without conditional compilation.
     */
    struct SolidSyslogAddress* AddressFake_Get(void);

EXTERN_C_END

#endif /* ADDRESSFAKE_H */
