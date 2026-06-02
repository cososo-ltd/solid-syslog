#ifndef SOLIDSYSLOGTLSSTREAMCATEGORIES_H
#define SOLIDSYSLOGTLSSTREAMCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/*
 * Portable TLS-stream error categories, shared by every TLS backend (OpenSSL,
 * Mbed TLS, or an integrator's own). A portable handler switch on
 * event->Category reacts to a TLS init or handshake failure identically
 * regardless of which TLS library produced it.
 */
#define SOLIDSYSLOG_CAT_TLSSTREAM_INIT_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_TLSSTREAM_BASE + 1U))
#define SOLIDSYSLOG_CAT_TLSSTREAM_HANDSHAKE_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_TLSSTREAM_BASE + 2U))

#endif /* SOLIDSYSLOGTLSSTREAMCATEGORIES_H */
