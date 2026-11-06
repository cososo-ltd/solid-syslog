#ifndef BDDTARGETSTDERRERRORHANDLER_H
#define BDDTARGETSTDERRERRORHANDLER_H

#include <stdbool.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorEvent;
    struct SolidSyslogErrorSource;

    void BddTargetStderrErrorHandler_Install(void);

    /* Which source counts as the TLS-stream role, so its reports can be marked
       `role=tls`. Injected rather than looked up: the handler links into targets
       and test binaries that have no TLS pack at all. NULL marks nothing. */
    void BddTargetStderrErrorHandler_SetTlsSource(const struct SolidSyslogErrorSource* source);

    /* `errors-fatal 0|1` - whether an ERROR or worse ends the process. On by
       default, because most scenarios treat one as a failed run. The matrix
       turns it off: a refused handshake is reported at ERROR, so a fatal
       handler would kill the target before the scenario could see what
       happened next. Returns false for any other name, so a caller can offer
       the value to the next handler. */
    bool BddTargetStderrErrorHandler_SetByName(const char* name, const char* value);

    /* Whether the handler ends the process on this event: an ERROR or worse
       while errors-fatal is on, other than the logger's halt. The halt
       callback is the target's reaction to a halt and is called after the
       halt is raised, so ending the process here would stop it being reached. */
    bool BddTargetStderrErrorHandler_IsFatal(const struct SolidSyslogErrorEvent* event);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETSTDERRERRORHANDLER_H */
