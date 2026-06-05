#ifndef SOLIDSYSLOGKEYFUNCTION_H
#define SOLIDSYSLOGKEYFUNCTION_H

#include "ExternC.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

EXTERN_C_BEGIN

    /* Fetches a secret key on demand. A keyed SecurityPolicy calls this at
     * seal / verify time, copies the key into a transient buffer, uses it, and
     * wipes the buffer — the key is never stored on the policy instance. The
     * integrator decides where the key actually lives (secure element, KDF,
     * encrypted NVM, ...).
     *
     * Writes up to `capacity` bytes into `keyOut`, sets `*keyLengthOut` to the
     * number of bytes written, and returns true on success. Returns false if
     * the key is unavailable or does not fit, which fails the seal / verify
     * operation closed.
     *
     * Each keyed policy imposes its own length requirement on the reported key
     * and fails closed if it is not met: AES-GCM requires exactly 32 bytes; the
     * HMAC-SHA256 policies require at least 32 bytes (the SHA-256 output size,
     * per RFC 2104 / NIST SP 800-107). */
    typedef bool (*SolidSyslogKeyFunction)(void* context, uint8_t* keyOut, size_t capacity, size_t* keyLengthOut);

EXTERN_C_END

#endif /* SOLIDSYSLOGKEYFUNCTION_H */
