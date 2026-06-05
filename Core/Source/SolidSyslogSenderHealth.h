#ifndef SOLIDSYSLOGSENDERHEALTH_H
#define SOLIDSYSLOGSENDERHEALTH_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /*
     * Per-sender reporting identity: which error source raised the edge and the
     * per-class detail codes for the two transitions. The portable Sender-role
     * categories and the severity ladder are owned by
     * SolidSyslogSenderHealth_Update — a single authoritative mapping every
     * sender shares — so a caller only supplies what is genuinely its own.
     */
    struct SolidSyslogSenderHealthReporter
    {
        const struct SolidSyslogErrorSource* Source;
        int32_t FailedDetail;
        int32_t RestoredDetail;
    };

    /*
     * Edge-triggered delivery-health reporting, shared by the Senders that can
     * observe whether a send reached the destination (StreamSender, UdpSender).
     *
     * The caller owns one DeliveryHealthy bit, initialised true. Each public
     * Send passes its boolean delivery result here:
     *   - a true -> false transition raises one DELIVERY_FAILED (ERR) event;
     *   - a false -> true transition raises one DELIVERY_RESTORED (NOTICE) event;
     *   - staying down or staying up raises nothing (anti-flood).
     */
    void SolidSyslogSenderHealth_Update(
        bool* healthy,
        bool delivered,
        const struct SolidSyslogSenderHealthReporter* reporter
    );

EXTERN_C_END

#endif /* SOLIDSYSLOGSENDERHEALTH_H */
