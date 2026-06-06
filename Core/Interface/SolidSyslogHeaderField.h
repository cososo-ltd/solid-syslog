#ifndef SOLIDSYSLOGHEADERFIELD_H
#define SOLIDSYSLOGHEADERFIELD_H

#include "ExternC.h"

#include <stddef.h>
#include <stdint.h>

EXTERN_C_BEGIN

    /* The value sink for an RFC 5424 header field (HOSTNAME / APP-NAME /
     * PROCID). A field producer is handed only a SolidSyslogHeaderField* — it
     * can append PRINTUSASCII content (printable US-ASCII; any other byte,
     * space included, is substituted) bounded to the field width, but cannot
     * reach the raw formatter. Stack-transient, no pool (D.002). */
    struct SolidSyslogHeaderField;

    /* Appends up to maxLength bytes of source (stopping at a NUL terminator),
     * substituting any byte outside printable US-ASCII ('!'..'~') — space
     * included — with the library's substitute character. Output is further
     * bounded by the field width the writer was created with. */
    void SolidSyslogHeaderField_PrintUsAscii(
        struct SolidSyslogHeaderField * field,
        const char* source,
        size_t maxLength
    );

    /* Appends the decimal digits of value (always printable US-ASCII), bounded
     * by the field width. */
    void SolidSyslogHeaderField_Uint32(struct SolidSyslogHeaderField * field, uint32_t value);

EXTERN_C_END

#endif /* SOLIDSYSLOGHEADERFIELD_H */
