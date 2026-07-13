/** @file
 *  Error codes and Source identity for the TlsStream adapter. */
#ifndef SOLIDSYSLOGTLSSTREAMERRORS_H
#define SOLIDSYSLOGTLSSTREAMERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is TlsStreamErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogTlsStreamErrors
    {
        TLSSTREAM_ERROR_POOL_EXHAUSTED,
        TLSSTREAM_ERROR_UNKNOWN_DESTROY,
        TLSSTREAM_ERROR_CONTEXT_INIT_FAILED,
        TLSSTREAM_ERROR_SESSION_INIT_FAILED,
        TLSSTREAM_ERROR_SERVER_NAME_NOT_SET,
        TLSSTREAM_ERROR_HANDSHAKE_REJECTED,
        TLSSTREAM_ERROR_HANDSHAKE_TIMEOUT,
        TLSSTREAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a TlsStream. A handler matches by address
     *  (event->Source == &TlsStreamErrorSource), then reads event->Detail as an
     *  enum SolidSyslogTlsStreamErrors. */
    extern const struct SolidSyslogErrorSource TlsStreamErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGTLSSTREAMERRORS_H */
