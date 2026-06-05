#ifndef SOLIDSYSLOGSDELEMENTPRIVATE_H
#define SOLIDSYSLOGSDELEMENTPRIVATE_H

#include "ExternC.h"

#include "SolidSyslogSdElement.h"
#include "SolidSyslogSdValuePrivate.h"

EXTERN_C_BEGIN

    struct SolidSyslogFormatter;

    /* Definition lives here (not the public header) so an SD author handed a
     * SolidSyslogSdElement* cannot reach the wrapped formatter. The embedded
     * Value is the sink _Param hands back — one per element, re-initialised on
     * each _Param (only one param value is open at a time). */
    struct SolidSyslogSdElement
    {
        struct SolidSyslogFormatter* Formatter;
        struct SolidSyslogSdValue Value;
    };

    /* Internal constructor — wraps a message-buffer formatter. MessageFormatter
     * (S14.06) builds one of these around the handed formatter and passes it to
     * each SD's Format. Stack-transient: the caller owns the storage. */
    void SolidSyslogSdElement_FromFormatter(struct SolidSyslogSdElement * element, struct SolidSyslogFormatter * formatter);

EXTERN_C_END

#endif /* SOLIDSYSLOGSDELEMENTPRIVATE_H */
