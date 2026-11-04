#ifndef WOLFSSLTESTLIBRARY_H
#define WOLFSSLTESTLIBRARY_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The process-global wolfSSL calls a test makes, behind a C seam because
       wolfssl/ssl.h is not clean under C++ -Wpedantic. */
    void WolfSslTestLibrary_Init(void);
    void WolfSslTestLibrary_Cleanup(void);
    const char* WolfSslTestLibrary_Version(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* WOLFSSLTESTLIBRARY_H */
