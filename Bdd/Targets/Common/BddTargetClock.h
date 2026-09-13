#ifndef BDDTARGETCLOCK_H
#define BDDTARGETCLOCK_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

/* The wall clock the target hands to mbedTLS so certificate validity is
   enforced on a device with no RTC. Seeded at build time and advanced by the
   scheduler's own uptime, which is enough to place the device inside or outside
   a certificate's validity window - the only question X.509 asks of a clock.

   This is deliberately not the clock the library logs with: SolidSyslogConfig
   leaves Clock NULL on these targets, so a record still carries NILVALUE and
   the time-quality scenarios still describe a device with no RTC. A device that
   is fed time by SNTP and has no battery-backed RTC is exactly this shape. */
#ifndef BDD_TARGET_BUILD_EPOCH
/* 2026-01-01T00:00:00Z. Overridden at configure time with the build's own
   instant; the fallback only has to sit inside the test material's window. */
#define BDD_TARGET_BUILD_EPOCH 1767225600
#endif

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Seconds the device has been running. */
    typedef uint32_t (*BddTargetClockUptimeFunction)(void);

    void BddTargetClock_Initialise(BddTargetClockUptimeFunction uptime);
    /* Seconds since the Unix epoch, as mbedTLS asks for them. */
    int64_t BddTargetClock_Now(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETCLOCK_H */
