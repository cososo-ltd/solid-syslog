#ifndef SOLIDSYSLOGTIMESTAMPFORMATTER_H
#define SOLIDSYSLOGTIMESTAMPFORMATTER_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogFormatter;
    struct SolidSyslogTimestamp;

    /* Formats a captured timestamp as an RFC 3339 / RFC 5424 TIMESTAMP field:
     * YYYY-MM-DDTHH:MM:SS.ffffff followed by 'Z' (UTC) or '±HH:MM'. A value
     * that violates any RFC range — including a zero-initialised timestamp —
     * is emitted as the NILVALUE '-'. Capturing the current time is the
     * caller's concern; this module only turns a value into bytes. */
    void SolidSyslogTimestampFormatter_Format(
        struct SolidSyslogFormatter * formatter,
        const struct SolidSyslogTimestamp* timestamp
    );

EXTERN_C_END

#endif /* SOLIDSYSLOGTIMESTAMPFORMATTER_H */
