/** @file
 *  Error codes and Source identity for the OpenSslStream adapter. */
#ifndef SOLIDSYSLOGOPENSSLSTREAMERRORS_H
#define SOLIDSYSLOGOPENSSLSTREAMERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is OpenSslStreamErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogOpenSslStreamErrors
    {
        OPENSSLSTREAM_ERROR_POOL_EXHAUSTED,
        OPENSSLSTREAM_ERROR_UNKNOWN_DESTROY,
        OPENSSLSTREAM_ERROR_CONTEXT_INIT_FAILED,
        OPENSSLSTREAM_ERROR_SESSION_INIT_FAILED,
        OPENSSLSTREAM_ERROR_SERVER_NAME_NOT_SET,
        OPENSSLSTREAM_ERROR_HANDSHAKE_REJECTED,
        OPENSSLSTREAM_ERROR_HANDSHAKE_TIMEOUT,
        OPENSSLSTREAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a OpenSslStream. A handler matches by address
     *  (event->Source == &OpenSslStreamErrorSource), then reads event->Detail as an
     *  enum SolidSyslogOpenSslStreamErrors. */
    extern const struct SolidSyslogErrorSource OpenSslStreamErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGOPENSSLSTREAMERRORS_H */
