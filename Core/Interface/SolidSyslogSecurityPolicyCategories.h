#ifndef SOLIDSYSLOGSECURITYPOLICYCATEGORIES_H
#define SOLIDSYSLOGSECURITYPOLICYCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/*
 * Portable SecurityPolicy-role error categories, shared by every integrity /
 * confidentiality policy (HMAC, AES-GCM, OpenSSL, Mbed TLS, ...). Seal (write
 * side) and open (read side) are kept distinct because "a stored record can no
 * longer be read back" is a materially different operational signal from
 * "a record could not be written".
 */
#define SOLIDSYSLOG_CAT_SECURITYPOLICY_KEY_UNAVAILABLE ((uint16_t) (SOLIDSYSLOG_CAT_SECURITYPOLICY_BASE + 1U))
#define SOLIDSYSLOG_CAT_SECURITYPOLICY_SEAL_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_SECURITYPOLICY_BASE + 2U))
#define SOLIDSYSLOG_CAT_SECURITYPOLICY_OPEN_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_SECURITYPOLICY_BASE + 3U))

#endif /* SOLIDSYSLOGSECURITYPOLICYCATEGORIES_H */
