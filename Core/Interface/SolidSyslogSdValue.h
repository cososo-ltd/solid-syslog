/** @file
 *  The per-param value sink of the SD authoring API: streams a PARAM value with
 *  RFC 5424 escaping applied by the library, so an author cannot break SD
 *  framing. */
#ifndef SOLIDSYSLOGSDVALUE_H
#define SOLIDSYSLOGSDVALUE_H

#include "SolidSyslogExternC.h"

#include <stddef.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** The per-param value sink of the SD authoring API. A value producer is
     *  handed only a SolidSyslogSdValue*, so it can stream escaped, UTF-8-safe
     *  content into the message buffer but cannot reach the raw formatter, open
     *  a param, or break SD framing. Stack-transient, no pool (D.002). */
    struct SolidSyslogSdValue;

    /** Streams @p source (a NUL-terminated UTF-8 chunk) into the value, escaping
     *  each of '"', '\\', ']' and substituting ill-formed UTF-8 with U+FFFD. May
     *  be called repeatedly on the same value; a multi-byte codepoint split
     *  across two calls is reassembled. Output is bounded by the message buffer. */
    void SolidSyslogSdValue_String(struct SolidSyslogSdValue * value, const char* source);

    /** As SolidSyslogSdValue_String, but caps the value at @p maxDecodedLength
     *  decoded bytes, for SD params a receiver parses into a width-limited
     *  field. The bound counts what the reader's un-escaping decoder extracts
     *  (an escape pair as one byte, a substituted U+FFFD as three), not the
     *  on-wire byte count. */
    void SolidSyslogSdValue_BoundedString(
        struct SolidSyslogSdValue * value,
        const char* source,
        size_t maxDecodedLength
    );

    /** Emits the decimal digits of @p number. Digits are never escapable, so no
     *  escaping applies. Output is bounded by the message buffer. */
    void SolidSyslogSdValue_Uint32(struct SolidSyslogSdValue * value, uint32_t number);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSDVALUE_H */
