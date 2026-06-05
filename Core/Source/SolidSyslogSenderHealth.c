#include "SolidSyslogSenderHealth.h"

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSenderCategories.h"

void SolidSyslogSenderHealth_Update(
    bool* healthy,
    bool delivered,
    const struct SolidSyslogSenderHealthReporter* reporter
)
{
    bool healthChanged = (delivered != *healthy);
    if (healthChanged)
    {
        *healthy = delivered;
        if (delivered)
        {
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_NOTICE,
                reporter->Source,
                SOLIDSYSLOG_CAT_SENDER_DELIVERY_RESTORED,
                reporter->RestoredDetail
            );
        }
        else
        {
            /* WARNING: a destination outage is recoverable (store-and-forward
             * covers it) and may clear on its own — it is not a library fault.
             * Setup faults an integrator must fix in code (bad config, pool
             * exhaustion) are CRITICAL; see docs/error-severity.md. */
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_WARNING,
                reporter->Source,
                SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED,
                reporter->FailedDetail
            );
        }
    }
}
