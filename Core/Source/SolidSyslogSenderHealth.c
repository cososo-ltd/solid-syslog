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
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_ERROR,
                reporter->Source,
                SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED,
                reporter->FailedDetail
            );
        }
    }
}
