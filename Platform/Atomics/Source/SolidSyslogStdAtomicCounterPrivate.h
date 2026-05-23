#ifndef SOLIDSYSLOGSTDATOMICCOUNTERPRIVATE_H
#define SOLIDSYSLOGSTDATOMICCOUNTERPRIVATE_H

#include <stdatomic.h>
#include <stdint.h>

#include "SolidSyslogAtomicCounterDefinition.h"

struct SolidSyslogStdAtomicCounter
{
    struct SolidSyslogAtomicCounter Base;
    _Atomic uint32_t Value;
};

void StdAtomicCounter_Initialise(struct SolidSyslogAtomicCounter* base);
void StdAtomicCounter_Cleanup(struct SolidSyslogAtomicCounter* base);

#endif /* SOLIDSYSLOGSTDATOMICCOUNTERPRIVATE_H */
