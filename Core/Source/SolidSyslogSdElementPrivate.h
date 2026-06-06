#ifndef SOLIDSYSLOGSDELEMENTPRIVATE_H
#define SOLIDSYSLOGSDELEMENTPRIVATE_H

#include "ExternC.h"

#include <stdbool.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogSdElement.h"
#include "SolidSyslogSdValuePrivate.h"

EXTERN_C_BEGIN

    /* Definition lives here (not the public header) so an SD author handed a
     * SolidSyslogSdElement* cannot reach the wrapped formatter. The embedded
     * Value is the sink _Param hands back — one per element, re-initialised on
     * each _Param (only one param value is open at a time).
     *
     * DropStorage backs a zero-size formatter that safely absorbs the value of
     * a skipped param (NULL param name, or a NULL-SD-ID-suppressed element):
     * every write to it is dropped, so a skipped value cannot corrupt framing.
     * Suppressed marks an element opened with a NULL SD-ID — it emits nothing. */
    struct SolidSyslogSdElement
    {
        struct SolidSyslogFormatter* Formatter;
        struct SolidSyslogSdValue Value;
        SolidSyslogFormatterStorage DropStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(0U)];
        struct SolidSyslogFormatter* DropFormatter;
        bool ValueOpen;
        bool Suppressed;
    };

    /* Internal constructor — wraps a message-buffer formatter. MessageFormatter
     * (S14.06) builds one of these around the handed formatter and passes it to
     * each SD's Format. Stack-transient: the caller owns the storage. */
    void SolidSyslogSdElement_FromFormatter(
        struct SolidSyslogSdElement * element,
        struct SolidSyslogFormatter * formatter
    );

EXTERN_C_END

#endif /* SOLIDSYSLOGSDELEMENTPRIVATE_H */
