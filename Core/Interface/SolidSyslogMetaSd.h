/** @file
 *  A StructuredData source for the RFC 5424 §7.3 "meta" SD-ELEMENT (IANA SD-ID,
 *  so no enterprise-number suffix), emitted on every message the owning logger
 *  formats. It always carries sequenceId, taken from one Increment of the
 *  injected AtomicCounter per message, and optionally sysUpTime and language.
 *  The two optional PARAMs are independently omitted when their config member is
 *  NULL, so a bare meta element is just [meta sequenceId="..."]. */
#ifndef SOLIDSYSLOGMETASD_H
#define SOLIDSYSLOGMETASD_H

#include "ExternC.h"
#include "SolidSyslogSdValueFunction.h"

#include <stdint.h>

EXTERN_C_BEGIN

    struct SolidSyslogAtomicCounter;
    struct SolidSyslogStructuredData;

    /** Returns system uptime in hundredths of a second (RFC 3418 TimeTicks),
     *  wrapping on overflow. Feeds the meta element's sysUpTime PARAM. */
    typedef uint32_t (*SolidSyslogSysUpTimeFunction)(void);

    /** Wiring for the "meta" SD-ELEMENT (RFC 5424 §7.3). Whatever is provided
     *  must outlive the created SD; the config is read only during Create. */
    struct SolidSyslogMetaSdConfig
    {
        /** Required; each message consumes one Increment as the sequenceId PARAM. */
        struct SolidSyslogAtomicCounter* Counter;
        SolidSyslogSysUpTimeFunction GetSysUpTime; /**< NULL omits the sysUpTime PARAM. */
        SolidSyslogSdValueFunction GetLanguage; /**< NULL omits the language PARAM. */
        void* LanguageContext; /**< Passed to GetLanguage unchanged. */
    };

    /** Create a meta SD source, emitted on every message the owning logger
     *  formats. Never returns NULL: a NULL config, a NULL Counter, or an
     *  exhausted pool reports via SolidSyslog_Error and returns the shared
     *  no-op NullSd, so callers need not null-check the result. */
    struct SolidSyslogStructuredData* SolidSyslogMetaSd_Create(const struct SolidSyslogMetaSdConfig* config);
    /** Release the SD's pool slot. Does not touch the injected Counter; the
     *  caller owns it. */
    void SolidSyslogMetaSd_Destroy(struct SolidSyslogStructuredData * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGMETASD_H */
