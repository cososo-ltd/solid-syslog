#include "WolfSslTestLibrary.h"

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <wolfssl/ssl.h>

void WolfSslTestLibrary_Init(void)
{
    (void) wolfSSL_Init();
}

void WolfSslTestLibrary_Cleanup(void)
{
    (void) wolfSSL_Cleanup();
}

const char* WolfSslTestLibrary_Version(void)
{
    return wolfSSL_lib_version();
}
