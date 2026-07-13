/** @file
 *  Error codes and Source identity for the OpenSslAesGcmPolicy adapter. */
#ifndef SOLIDSYSLOGOPENSSLAESGCMPOLICYERRORS_H
#define SOLIDSYSLOGOPENSSLAESGCMPOLICYERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is OpenSslAesGcmPolicyErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogOpenSslAesGcmPolicyErrors
    {
        OPENSSLAESGCMPOLICY_ERROR_POOL_EXHAUSTED,
        OPENSSLAESGCMPOLICY_ERROR_UNKNOWN_DESTROY,
        OPENSSLAESGCMPOLICY_ERROR_BAD_CONFIG,
        OPENSSLAESGCMPOLICY_ERROR_KEY_UNAVAILABLE,
        OPENSSLAESGCMPOLICY_ERROR_NONCE_FAILED,
        OPENSSLAESGCMPOLICY_ERROR_ENCRYPT_FAILED,
        OPENSSLAESGCMPOLICY_ERROR_DECRYPT_FAILED,
        OPENSSLAESGCMPOLICY_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by an OpenSslAesGcmPolicy. A handler matches by
     *  address (event->Source == &OpenSslAesGcmPolicyErrorSource), then reads
     *  event->Detail as an enum SolidSyslogOpenSslAesGcmPolicyErrors. */
    extern const struct SolidSyslogErrorSource OpenSslAesGcmPolicyErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGOPENSSLAESGCMPOLICYERRORS_H */
