/** @file
 *  Portable category constants (uint16_t macros) for the SecurityPolicy role:
 *  SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE / _SEAL_FAILED / _OPEN_FAILED. */
#ifndef SOLIDSYSLOGSECURITYPOLICYCATEGORIES_H
#define SOLIDSYSLOGSECURITYPOLICYCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/**
 * Portable SecurityPolicy-role error categories, shared by every integrity /
 * confidentiality policy (HMAC, AES-GCM, OpenSSL, Mbed TLS, ...). Seal (write
 * side) and open (read side) are kept distinct because "a stored record can no
 * longer be read back" is a materially different operational signal from
 * "a record could not be written".
 */

/** The policy's key material was missing or could not be loaded. */
#define SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE ((uint16_t) (SOLIDSYSLOG_CAT_SECURITY_POLICY_BASE + 1U))
/** Write side: a record could not be sealed (tag / encrypt). */
#define SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_SECURITY_POLICY_BASE + 2U))
/** Read side: a stored record failed to open (verify / decrypt) and cannot be read back. */
#define SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_SECURITY_POLICY_BASE + 3U))

#endif /* SOLIDSYSLOGSECURITYPOLICYCATEGORIES_H */
