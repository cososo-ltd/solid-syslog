#ifndef SOLIDSYSLOGPOSIXCLOCK_H
#define SOLIDSYSLOGPOSIXCLOCK_H

#include "ExternC.h"

struct SolidSyslogTimestamp;

EXTERN_C_BEGIN

    void SolidSyslogPosixClock_GetTimestamp(struct SolidSyslogTimestamp * timestamp);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXCLOCK_H */
