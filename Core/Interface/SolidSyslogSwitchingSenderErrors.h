/** @file
 *  Error codes and Source identity for the SwitchingSender. */
#ifndef SOLIDSYSLOGSWITCHINGSENDERERRORS_H
#define SOLIDSYSLOGSWITCHINGSENDERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SwitchingSenderErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogSwitchingSenderErrors
    {
        SWITCHINGSENDER_ERROR_NULL_CONFIG,
        SWITCHINGSENDER_ERROR_NULL_SENDERS,
        SWITCHINGSENDER_ERROR_NULL_SELECTOR,
        SWITCHINGSENDER_ERROR_POOL_EXHAUSTED,
        SWITCHINGSENDER_ERROR_UNKNOWN_DESTROY,
        SWITCHINGSENDER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by a SwitchingSender. A handler matches by
     *  address (event->Source == &SwitchingSenderErrorSource), then reads
     *  event->Detail as an enum SolidSyslogSwitchingSenderErrors. */
    extern const struct SolidSyslogErrorSource SwitchingSenderErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSWITCHINGSENDERERRORS_H */
