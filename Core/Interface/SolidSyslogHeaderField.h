#ifndef SOLIDSYSLOGHEADERFIELD_H
#define SOLIDSYSLOGHEADERFIELD_H

#include "ExternC.h"

#include <stddef.h>
#include <stdint.h>

EXTERN_C_BEGIN

    /** The value sink for an RFC 5424 header field (HOSTNAME / APP-NAME /
     *  PROCID). A field producer is handed only a SolidSyslogHeaderField*: it
     *  can append content bounded to the field width, but cannot reach the raw
     *  formatter or break the header framing. The sink owns the charset,
     *  substituting any byte outside printable US-ASCII (space included), so a
     *  producer cannot emit a byte that would corrupt the header.
     *  Stack-transient, no pool (D.002). */
    struct SolidSyslogHeaderField;

    /** Appends up to @p maxLength bytes of @p source (stopping at a NUL
     *  terminator), substituting any byte outside printable US-ASCII
     *  ('!'..'~'), space included, with the library's substitute character.
     *  Further bounded by the field width the sink was created with, shared
     *  across all appends to this field. */
    void SolidSyslogHeaderField_PrintUsAscii(
        struct SolidSyslogHeaderField * field,
        const char* source,
        size_t maxLength
    );

    /** Appends the decimal digits of @p value (always printable US-ASCII),
     *  bounded by the same field width. */
    void SolidSyslogHeaderField_Uint32(struct SolidSyslogHeaderField * field, uint32_t value);

EXTERN_C_END

#endif /* SOLIDSYSLOGHEADERFIELD_H */
