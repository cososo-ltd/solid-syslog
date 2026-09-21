/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable category constants (uint16_t macros) for the Stream role:
 *  SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED and
 *  SOLIDSYSLOG_CAT_STREAM_OPTION_REFUSED. */
#ifndef SOLIDSYSLOGSTREAMCATEGORIES_H
#define SOLIDSYSLOGSTREAMCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/**
 * Portable Stream-role error categories. Any Stream implementation reuses
 * these; a portable handler switch on event->Category works identically
 * across every stream backend.
 */

/** The stream could not establish its connection to the destination. Raised
 *  once per attempt, so a destination that stays down produces one event per
 *  Service pass; the edge-triggered summary of an outage is the Sender's
 *  SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED, not this. event->Detail names which
 *  step failed. */
#define SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_STREAM_BASE + 1U))

/** The connection stands, but the stack declined a socket option the stream
 *  asked for, so the connection is less robust than intended - a keepalive that
 *  will not detect a dead peer, or a latency setting that did not take. Raised
 *  once per connection attempt however many options were declined: the
 *  engineer's next step is the same whichever one it was, and repeating it per
 *  option would say nothing more. */
#define SOLIDSYSLOG_CAT_STREAM_OPTION_REFUSED ((uint16_t) (SOLIDSYSLOG_CAT_STREAM_BASE + 2U))

#endif /* SOLIDSYSLOGSTREAMCATEGORIES_H */
