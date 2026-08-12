/** @file
 *  Consumer smoke test (S30.04): the smallest translation unit that proves an
 *  integrator can include the public headers, call the API and link against the
 *  SolidSyslog target their FetchContent pulled in. Nothing is delivered - the
 *  Null buffer swallows the record - because the build, not the behaviour, is
 *  what this checks. */

#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogPrival.h"

int main(void)
{
    struct SolidSyslogConfig config = {0};
    struct SolidSyslogMessage message = {0};
    struct SolidSyslog* syslog = NULL;

    config.Buffer = SolidSyslogNullBuffer_Get();
    syslog = SolidSyslog_Create(&config);

    message.Facility = SOLIDSYSLOG_FACILITY_USER;
    message.Severity = SOLIDSYSLOG_SEVERITY_INFORMATIONAL;
    message.MessageId = "SMOKE";
    message.Msg = "consumer smoke test";

    SolidSyslog_Log(syslog, &message);
    (void) SolidSyslog_Service(syslog);
    SolidSyslog_Destroy(syslog);

    return 0;
}
