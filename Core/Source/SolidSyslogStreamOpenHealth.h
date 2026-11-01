/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGSTREAMOPENHEALTH_H
#define SOLIDSYSLOGSTREAMOPENHEALTH_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogPrival.h"

#include <stdbool.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /*
     * Once-per-run reporting of a failed Open, shared by every stream backend.
     *
     * The sender calls Open on every service pass while the destination is
     * down, and the sender's own DELIVERY_FAILED already marks the edge, so a
     * stream reporting every attempt floods the handler with the same fact. A
     * stream holds one of these, zero-initialised, and reports every failed
     * Open through it:
     *   - the first failure in a run reports, with its severity, detail and
     *     native code, which is the diagnostic the sender's edge does not carry;
     *   - a repeat with the same detail reports nothing, whatever its native code;
     *   - a different detail is new information and reports;
     *   - Opened, called when an Open succeeds, re-arms it.
     * There is no restored event: the sender's DELIVERY_RESTORED is that.
     */
    struct SolidSyslogStreamOpenHealth
    {
        bool Failing;
        int32_t FailedDetail;
    };

    void SolidSyslogStreamOpenHealth_ReportFailure(
        struct SolidSyslogStreamOpenHealth * self,
        const struct SolidSyslogErrorSource* source,
        enum SolidSyslogSeverity severity,
        uint16_t category,
        int32_t detail
    );

    void SolidSyslogStreamOpenHealth_ReportFailureWithNativeCode(
        struct SolidSyslogStreamOpenHealth * self,
        const struct SolidSyslogErrorSource* source,
        enum SolidSyslogSeverity severity,
        uint16_t category,
        int32_t detail,
        int32_t nativeCode
    );

    void SolidSyslogStreamOpenHealth_Opened(struct SolidSyslogStreamOpenHealth * self);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSTREAMOPENHEALTH_H */
