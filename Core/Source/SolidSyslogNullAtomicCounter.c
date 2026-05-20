#include "SolidSyslogNullAtomicCounter.h"

#include <stdint.h>

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogAtomicCounterDefinition.h"

static uint32_t NullAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base);

static struct SolidSyslogAtomicCounter instance = {.Increment = NullAtomicCounter_Increment};

struct SolidSyslogAtomicCounter* SolidSyslogNullAtomicCounter_Get(void)
{
    return &instance;
}

/* Returns 1U unconditionally. RFC 5424 §7.3.1 requires sequenceIds in
 * [1, 2^31 - 1] and never 0; 1U is indistinguishable from the post-power-on
 * or post-wrap state, which is the safest fallback when an integrator's
 * AtomicCounter pool is exhausted. */
static uint32_t NullAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base)
{
    (void) base;
    return 1U;
}
