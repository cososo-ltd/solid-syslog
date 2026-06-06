#ifndef SOLIDSYSLOGHEADERFIELDPRIVATE_H
#define SOLIDSYSLOGHEADERFIELDPRIVATE_H

#include "ExternC.h"

#include <stddef.h>

#include "SolidSyslogHeaderField.h"

EXTERN_C_BEGIN

    struct SolidSyslogFormatter;

    /* Definition lives here (not the public header) so a field producer handed
     * a SolidSyslogHeaderField* cannot reach the wrapped formatter. Remaining
     * is the field-width budget still available across this field's appends. */
    struct SolidSyslogHeaderField
    {
        struct SolidSyslogFormatter* Formatter;
        size_t Remaining;
    };

    /* Internal constructor — wraps a message-buffer formatter and caps this
     * field at maxLength bytes. MessageFormatter builds one per header field
     * and passes it to the configured callback. Stack-transient: the caller
     * owns the storage. */
    void SolidSyslogHeaderField_FromFormatter(
        struct SolidSyslogHeaderField * field,
        struct SolidSyslogFormatter * formatter,
        size_t maxLength
    );

EXTERN_C_END

#endif /* SOLIDSYSLOGHEADERFIELDPRIVATE_H */
