/** @file
 *  Error codes and Source identity for the MbedTlsStream adapter. */
#ifndef SOLIDSYSLOGMBEDTLSSTREAMERRORS_H
#define SOLIDSYSLOGMBEDTLSSTREAMERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is MbedTlsStreamErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. HANDSHAKE_REJECTED and HANDSHAKE_TIMEOUT are distinct
     *  so a handler can tell a peer that refused the handshake from one that never
     *  finished it within the bounded budget. */
    enum SolidSyslogMbedTlsStreamErrors
    {
        MBEDTLSSTREAM_ERROR_POOL_EXHAUSTED,
        MBEDTLSSTREAM_ERROR_UNKNOWN_DESTROY,
        MBEDTLSSTREAM_ERROR_DEFAULTS_NOT_APPLIED,
        MBEDTLSSTREAM_ERROR_SESSION_INIT_FAILED,
        MBEDTLSSTREAM_ERROR_SERVER_NAME_NOT_SET,
        MBEDTLSSTREAM_ERROR_HANDSHAKE_REJECTED,
        MBEDTLSSTREAM_ERROR_HANDSHAKE_TIMEOUT,
        MBEDTLSSTREAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by an MbedTlsStream. A handler matches by address
     *  (event->Source == &MbedTlsStreamErrorSource), then reads event->Detail as an
     *  enum SolidSyslogMbedTlsStreamErrors. */
    extern const struct SolidSyslogErrorSource MbedTlsStreamErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGMBEDTLSSTREAMERRORS_H */
