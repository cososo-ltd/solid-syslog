#ifndef SOLIDSYSLOGERROR_H
#define SOLIDSYSLOGERROR_H

#include <stdint.h>

#include "ExternC.h"
#include "SolidSyslogPrival.h"

/*
 * Policy severities for the universal lifecycle categories — one authoritative
 * level per category so the choice cannot drift across the dozens of emit
 * sites that raise them. See docs/error-severity.md. BAD_CONFIG is split: a
 * fatal misconfig (Create fell back to the Null object) uses the macro below,
 * while a degraded-but-delivering misconfig emits SOLIDSYSLOG_SEVERITY_WARNING
 * directly at the site — the two are genuinely different levels, so a single
 * shared macro would be a footgun.
 */
#define SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY SOLIDSYSLOG_SEVERITY_CRITICAL
#define SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY SOLIDSYSLOG_SEVERITY_CRITICAL
#define SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY SOLIDSYSLOG_SEVERITY_CRITICAL
#define SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY SOLIDSYSLOG_SEVERITY_WARNING

EXTERN_C_BEGIN

    /** The identity of an error-emitting class. There is one extern instance per
     *  class (e.g. SolidSyslogErrorSource); a handler recognises the emitter by
     *  matching Source against that instance's address, not by reading Name. */
    struct SolidSyslogErrorSource
    {
        const char* Name; /**< Human-readable tag for logging; not the match key. */
    };

    /** What a handler receives. The last three fields are orthogonal axes:
     *  Source is the extensible identity (match by pointer against the emitting
     *  class's extern), Category is the portable reaction axis
     *  (SolidSyslogErrorCategory.h) a handler can switch on without knowing the
     *  Source, and Detail is the fine-grained code within that Source: today the
     *  per-class enum SolidSyslog<Class>Errors value, later a native code such as
     *  errno or an X509_V_ERR_*. Severity is an independent urgency axis (how bad,
     *  now), not who-must-fix; see docs/error-severity.md. */
    struct SolidSyslogErrorEvent
    {
        enum SolidSyslogSeverity Severity;
        const struct SolidSyslogErrorSource* Source;
        uint16_t Category;
        int32_t Detail;
    };

    /** Installed via SolidSyslog_SetErrorHandler; @p context is passed back
     *  unchanged. The event is only valid for the duration of the call. */
    typedef void (*SolidSyslogErrorHandler)(void* context, const struct SolidSyslogErrorEvent* event);

    /** Install the single global error handler. Intended for setup-time
     *  configuration: the slot is one shared pointer, not synchronised against
     *  concurrent SolidSyslog_Error emissions. The default handler is a silent
     *  no-op; passing @p handler = NULL restores it. */
    void SolidSyslog_SetErrorHandler(SolidSyslogErrorHandler handler, void* context);

    /** Emit an event to the installed handler. Called from inside the library at
     *  fault sites; integrators building their own Sources call it too. */
    void SolidSyslog_Error(
        enum SolidSyslogSeverity severity,
        const struct SolidSyslogErrorSource* source,
        uint16_t category,
        int32_t detail
    );

EXTERN_C_END

#endif /* SOLIDSYSLOGERROR_H */
