#include "PendingEventsDrain.h"

#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPendingEvents.h"

void PendingEventsDrain_All(void)
{
    struct SolidSyslogErrorEvent discarded;
    for (size_t index = 0U; index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT; index++)
    {
        (void) SolidSyslogPendingEvents_Take(index, &discarded);
    }
}
