#ifndef SOLIDSYSLOGSDVALUEPRIVATE_H
#define SOLIDSYSLOGSDVALUEPRIVATE_H

#include "ExternC.h"

#include "SolidSyslogSdValue.h"

EXTERN_C_BEGIN

    struct SolidSyslogFormatter;

    enum
    {
        SDVALUE_MAX_CODEPOINT_BYTES = 4
    };

    /* Definition lives here (not the public header) so a producer handed a
     * SolidSyslogSdValue* cannot reach the wrapped formatter. SolidSyslogSdElement
     * (S14.02) embeds one of these and initialises it via _FromFormatter.
     *
     * Pending holds an incomplete trailing UTF-8 sequence (Option B streaming
     * state): when a _String chunk ends mid-codepoint the leading bytes wait
     * here for the next chunk's continuation bytes to complete them. */
    struct SolidSyslogSdValue
    {
        struct SolidSyslogFormatter* Formatter;
        char Pending[SDVALUE_MAX_CODEPOINT_BYTES];
        size_t PendingCount;
    };

    /* Internal constructor — wraps a message-buffer formatter so values stream
     * straight into it. Stack-transient: the caller owns the storage. */
    void SolidSyslogSdValue_FromFormatter(struct SolidSyslogSdValue * value, struct SolidSyslogFormatter * formatter);

    /* Closes the value: a still-incomplete trailing UTF-8 sequence held in
     * Pending is flushed as a single U+FFFD. SolidSyslogSdElement (S14.02)
     * calls this when the param's value ends. */
    void SolidSyslogSdValue_Close(struct SolidSyslogSdValue * value);

EXTERN_C_END

#endif /* SOLIDSYSLOGSDVALUEPRIVATE_H */
