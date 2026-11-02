#include "CounterFake.h"

#include <stdint.h>
#include <stdlib.h>

#include "SolidSyslogAtomicCounterDefinition.h"

struct CounterFake
{
    struct SolidSyslogAtomicCounter Base;
    uint32_t Value;
};

static uint32_t Increment(struct SolidSyslogAtomicCounter* base)
{
    struct CounterFake* fake = (struct CounterFake*) base;
    fake->Value++;
    return fake->Value;
}

struct SolidSyslogAtomicCounter* CounterFake_Create(void)
{
    struct CounterFake* fake = (struct CounterFake*) calloc(1, sizeof(struct CounterFake));
    fake->Base.Increment = Increment;
    return &fake->Base;
}

void CounterFake_Destroy(struct SolidSyslogAtomicCounter* counter)
{
    free(counter);
}
