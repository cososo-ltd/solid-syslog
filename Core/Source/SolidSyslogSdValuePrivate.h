/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGSDVALUEPRIVATE_H
#define SOLIDSYSLOGSDVALUEPRIVATE_H

#include "SolidSyslogExternC.h"

#include "SolidSyslogSdValue.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogFormatter;

    enum
    {
        SDVALUE_MAX_CODEPOINT_BYTES = 4
    };

    /* Definition lives here (not the public header) so a producer handed a
     * SolidSyslogSdValue* cannot reach the wrapped formatter. SolidSyslogSdElement
     * (S14.02) embeds one of these and initialises it via
     * SolidSyslogSdValue_FromFormatter.
     *
     * Pending holds an incomplete trailing UTF-8 sequence (Option B streaming
     * state): when a SolidSyslogSdValue_String chunk ends mid-codepoint the
     * leading bytes wait
     * here for the next chunk's continuation bytes to complete them. */
    struct SolidSyslogSdValue
    {
        struct SolidSyslogFormatter* Formatter;
        char Pending[SDVALUE_MAX_CODEPOINT_BYTES];
        size_t PendingCount;
    };

    /* Internal constructor - wraps a message-buffer formatter so values stream
     * straight into it. Stack-transient: the caller owns the storage. */
    void SolidSyslogSdValue_FromFormatter(struct SolidSyslogSdValue * value, struct SolidSyslogFormatter * formatter);

    /* Closes the value: a still-incomplete trailing UTF-8 sequence held in
     * Pending is flushed as a single U+FFFD. SolidSyslogSdElement (S14.02)
     * calls this when the param's value ends. */
    void SolidSyslogSdValue_Close(struct SolidSyslogSdValue * value);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSDVALUEPRIVATE_H */
