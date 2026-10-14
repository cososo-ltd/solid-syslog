/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable category constants (uint16_t macros) for the SecurityPolicy role:
 *  SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE / _SEAL_FAILED / _OPEN_FAILED. */
#ifndef SOLIDSYSLOGSECURITYPOLICYCATEGORIES_H
#define SOLIDSYSLOGSECURITYPOLICYCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/**
 * Portable SecurityPolicy-role error categories, shared by every integrity /
 * confidentiality policy whichever library provides it. Seal (write
 * side) and open (read side) are kept distinct because a record that could not
 * be sealed is lost, while one that could not be opened stays in the store and
 * is tried again.
 */

/** The policy's key material was missing or could not be loaded. */
#define SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE ((uint16_t) (SOLIDSYSLOG_CAT_SECURITY_POLICY_BASE + 1U))
/** Write side: a record could not be sealed (tag / encrypt). */
#define SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_SECURITY_POLICY_BASE + 2U))
/** Read side: the crypto backend failed while verifying or decrypting a stored
 *  record. This is a fault in the policy, not a verdict on the record, which is
 *  kept and tried again. A record that fails verification raises nothing here:
 *  the store reports its discard. */
#define SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_SECURITY_POLICY_BASE + 3U))

#endif /* SOLIDSYSLOGSECURITYPOLICYCATEGORIES_H */
