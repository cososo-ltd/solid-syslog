/** @file
 *  The no-queue Buffer. Write forwards the record straight to the injected
 *  sender inline, so SolidSyslog_Log blocks on the send and returns only once it
 *  completes; Read always reports empty because nothing is ever queued, so
 *  Service has nothing to drain. This is the simplest wiring for a single-task
 *  setup with no store-and-forward — no ring, no mutex, no background drain. The
 *  cost is that a slow or blocking sender stalls the logging thread. */
#ifndef SOLIDSYSLOGPASSTHROUGHBUFFER_H
#define SOLIDSYSLOGPASSTHROUGHBUFFER_H

#include "ExternC.h"

EXTERN_C_BEGIN

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

EXTERN_C_END

#endif /* SOLIDSYSLOGPASSTHROUGHBUFFER_H */
