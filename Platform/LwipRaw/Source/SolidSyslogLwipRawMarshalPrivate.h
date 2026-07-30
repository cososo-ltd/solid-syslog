#ifndef SOLIDSYSLOGLWIPRAWMARSHALPRIVATE_H
#define SOLIDSYSLOGLWIPRAWMARSHALPRIVATE_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogLwipRawMarshal.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Library-internal dispatch call site. The LwipRaw wrapper classes batch
       the lwIP API calls for one public operation into a single callback and
       hand it here; we run it through the currently-installed marshal. No NULL
       check — the null-object default direct-calls. */
    void SolidSyslogLwipRaw_Marshal(SolidSyslogLwipRawCallback callback, void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWMARSHALPRIVATE_H */
