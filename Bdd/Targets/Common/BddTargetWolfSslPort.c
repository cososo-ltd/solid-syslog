/* The seed and the clock a wolfSSL built from source asks its target for. Both
   are named in the target's user_settings.h. */
#include "BddTargetWolfSslPort.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "BddTargetClock.h"
#include "BddTargetOsPrimitives.h"

int BddTargetWolfSslPort_GenerateSeed(unsigned char* output, unsigned int size);
time_t XTIME(time_t* timer);

static uint32_t BddTargetWolfSslPort_UptimeSeconds(void);

void BddTargetWolfSslPort_Initialise(void)
{
    BddTargetClock_Initialise(BddTargetWolfSslPort_UptimeSeconds);
    /* Said at every boot, so nobody carries this seed off the BDD target. */
    (void) printf("[wolfssl] WARNING: demo-only seed (uptime + per-call counter). Not for production.\r\n");
}

static uint32_t BddTargetWolfSslPort_UptimeSeconds(void)
{
    return (uint32_t) (BddTargetOsPrimitives_UptimeMilliseconds() / 1000U);
}

/* Demo-only: QEMU has no entropy source, so the seed mixes the uptime, a
   per-call counter and the destination address. wolfSSL's Hash-DRBG runs over
   it as it would over a real one; an integrator replaces this with a TRNG. */
int BddTargetWolfSslPort_GenerateSeed(unsigned char* output, unsigned int size)
{
    static uint32_t counter = 0U;
    for (unsigned int i = 0U; i < size; i++)
    {
        counter++;
        uint32_t mix = (uint32_t) BddTargetOsPrimitives_UptimeMilliseconds() ^ (counter * UINT32_C(2654435761)) ^
                       (uint32_t) (uintptr_t) &output[i];
        output[i] = (unsigned char) ((mix >> ((i % 4U) * 8U)) & 0xFFU);
    }
    return 0;
}

/* wolfSSL calls this by name under USER_TIME. The clock is seeded at build time
   and advanced by uptime, which is enough to place the device inside or outside
   a certificate's validity window. */
time_t XTIME(time_t* timer)
{
    time_t now = (time_t) BddTargetClock_Now();
    if (timer != NULL)
    {
        *timer = now;
    }
    return now;
}
