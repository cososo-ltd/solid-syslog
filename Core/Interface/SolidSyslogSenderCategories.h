/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable category constants (uint16_t macros) for the Sender role:
 *  SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED / _DELIVERY_RESTORED. */
#ifndef SOLIDSYSLOGSENDERCATEGORIES_H
#define SOLIDSYSLOGSENDERCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/**
 * Portable Sender-role error categories, shared by every Sender that can
 * observe whether a send reached the destination (StreamSender, UdpSender).
 * A portable handler switch on event->Category reacts to a delivery-health
 * transition identically regardless of which sender raised it; event->Source
 * still tells it which transport.
 */

/** Delivery to the destination started failing (edge into the failed state). */
#define SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_SENDER_BASE + 1U))
/** Delivery to the destination recovered after a failure (edge back to healthy). */
#define SOLIDSYSLOG_CAT_SENDER_DELIVERY_RESTORED ((uint16_t) (SOLIDSYSLOG_CAT_SENDER_BASE + 2U))
/** The sender discarded a record it could not deliver and told Service it was
 *  sent, so the record is gone rather than retried. event->Source names the
 *  sender and event->Detail why. Raised once per run of losses; the sender's
 *  detail code says what ends the run. */
#define SOLIDSYSLOG_CAT_SENDER_RECORDS_LOST ((uint16_t) (SOLIDSYSLOG_CAT_SENDER_BASE + 3U))

#endif /* SOLIDSYSLOGSENDERCATEGORIES_H */
