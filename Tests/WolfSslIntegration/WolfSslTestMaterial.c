#include "WolfSslTestMaterial.h"

#include <stdio.h>
#include <string.h>

bool WolfSslTestMaterial_Load(const char* name, struct WolfSslTestMaterial* out)
{
    (void) snprintf(out->Path, sizeof(out->Path), "%s/%s", WOLFSSL_TEST_MATERIAL_DIR, name);
    out->Length = 0U;
    FILE* file = fopen(out->Path, "rb");
    if (file != NULL)
    {
        out->Length = fread(out->Bytes, 1U, sizeof(out->Bytes), file);
        (void) fclose(file);
    }
    return out->Length > 0U;
}

void WolfSslTestMaterial_KeyNameFor(const char* certificateName, char* out, size_t capacity)
{
    size_t stem = strlen(certificateName) - strlen(".pem");
    (void) snprintf(out, capacity, "%.*s.key", (int) stem, certificateName);
}
