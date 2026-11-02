#include "WolfSslTestRng.h"

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <stdbool.h>
#include <stdlib.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/settings.h>

static WC_RNG rng;
static bool initialised;

struct WC_RNG* WolfSslTestRng_Get(void)
{
    if (!initialised)
    {
        if (wc_InitRng(&rng) != 0)
        {
            abort();
        }
        initialised = true;
    }
    return &rng;
}

void WolfSslTestRng_Release(void)
{
    if (initialised)
    {
        wc_FreeRng(&rng);
        initialised = false;
    }
}
