#ifndef WOLFSSLTESTMATERIAL_H
#define WOLFSSLTESTMATERIAL_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* A PEM file from the material generated at build time, read whole.
       The buffers outlive every connection made with them: the fixture holds
       them. */
    struct WolfSslTestMaterial
    {
        unsigned char Bytes[8192];
        size_t Length;
        char Path[512];
    };

    /* Reads `name` from WOLFSSL_TEST_MATERIAL_DIR. False where the file is
       missing or empty - the build-time generator did not run. */
    bool WolfSslTestMaterial_Load(const char* name, struct WolfSslTestMaterial* out);

    /* The key file that pairs with a certificate file: `server.pem` -> `server.key`. */
    void WolfSslTestMaterial_KeyNameFor(const char* certificateName, char* out, size_t capacity);

SOLIDSYSLOG_EXTERN_C_END

#endif /* WOLFSSLTESTMATERIAL_H */
