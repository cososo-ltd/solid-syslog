/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The no-queue Buffer. Write forwards the record straight to the injected
 *  sender inline, so SolidSyslog_Log blocks on the send and returns only once it
 *  completes; Read always reports empty because nothing is ever queued, so
 *  Service has nothing to drain. This is the simplest wiring for a single-task
 *  setup with no store-and-forward - no ring, no mutex, no background drain. The
 *  cost is that a slow or blocking sender stalls the logging thread.
 *
 *  Because Write sends on the task that logs, the gate
 *  SolidSyslogConfig.IsDestinationKnown holds in Service does not reach it: a
 *  device that logs before it knows where its collector is queues instead.
 *
 *  Service still makes the logger's own reports, and nothing else does, so the
 *  task that logs calls SolidSyslog_Service as well - after each Log at the
 *  least. A report the sender refuses is held and retried on every pass, and
 *  each retry is a send, so a call from the task's main loop lands it when the
 *  collector returns rather than at the device's next record. A store behind
 *  this buffer would hold only those reports: Write never reaches it, so the
 *  records the application logs are still lost in an outage. Wire the Null
 *  store; a device that stays on one task but wants a store puts a one-record
 *  SolidSyslogCircularBuffer over SolidSyslogNullMutex in front of it instead
 *  of this buffer, serviced the same way. */
#ifndef SOLIDSYSLOGPASSTHROUGHBUFFER_H
#define SOLIDSYSLOGPASSTHROUGHBUFFER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogSender;
    struct SolidSyslogBuffer;

    /** The no-queue Buffer: Write sends inline through @p sender, so
     *  SolidSyslog_Log blocks until the send returns and Service has nothing to
     *  drain. Suits single-task setups with no store-and-forward. @p sender must
     *  outlive the buffer. A NULL sender, or an exhausted pool, falls back to the
     *  shared NullBuffer. */
    struct SolidSyslogBuffer* SolidSyslogPassthroughBuffer_Create(struct SolidSyslogSender * sender);
    /** Release the pool slot; does not destroy the injected sender. */
    void SolidSyslogPassthroughBuffer_Destroy(struct SolidSyslogBuffer * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPASSTHROUGHBUFFER_H */
