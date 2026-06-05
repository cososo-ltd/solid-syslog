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
            /* WARNING, not ERROR: a destination outage is operator-fixable and
             * recoverable (store-and-forward covers it), unlike a library setup
             * fault (bad config / pool exhaustion) which an integrator must fix
             * in code. ERROR and above stays reserved for the latter. */
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_WARNING,
                reporter->Source,
                SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED,
                reporter->FailedDetail
            );
        }
    }
}
