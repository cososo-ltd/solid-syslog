/** @file
 *  A StructuredData source for the RFC 5424 §7.2 "origin" SD-ELEMENT (IANA
 *  SD-ID, so no enterprise-number suffix), emitted on every message the owning
 *  logger formats. It can carry software, swVersion, enterpriseId, and any
 *  number of repeated ip PARAMs. Every field is independently optional: a NULL
 *  string omits its PARAM, and the ip PARAMs appear only when both GetIpCount
 *  and GetIpAt are supplied (GetIpAt is then called once per index). The config
 *  strings are borrowed, not copied, and read at Format time — they must outlive
 *  the created SD. */
#ifndef SOLIDSYSLOGORIGINSD_H
#define SOLIDSYSLOGORIGINSD_H

#include "SolidSyslogExternC.h"

#include <stddef.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogSdValue;
    struct SolidSyslogStructuredData;

    /** Returns how many origin "ip" PARAMs to emit; GetIpAt is then called for
     *  each index in [0, count). @p context is the shared IpContext, passed
     *  through unchanged (the same value GetIpAt receives). */
    typedef size_t (*SolidSyslogOriginIpCountFunction)(void* context);
    /** Writes the @p index-th origin address into the @p value sink. @p context
     *  is passed through unchanged. */
    typedef void (*SolidSyslogOriginIpAtFunction)(struct SolidSyslogSdValue* value, void* context, size_t index);

    /** Wiring for the "origin" SD-ELEMENT (RFC 5424 §7.2). Every field is
     *  independently optional: a NULL string omits its PARAM, and the ip PARAMs
     *  are emitted only when both GetIpCount and GetIpAt are non-NULL. The
     *  string pointers are borrowed, not copied, and read at Format time, so
     *  they (and everything else provided) must outlive the created SD; the
     *  config struct itself is only read during Create. */
    struct SolidSyslogOriginSdConfig
    {
        /** NULL omits the software PARAM. RFC 5424 §7.2.3 bounds it at 48 characters
         *  and the value is carried as supplied, so observing that is the caller's
         *  until #748 enforces it. §7.2.3 also asks that it name the generating
         *  software rather than repeat APP-NAME. */
        const char* Software;
        /** NULL omits the swVersion PARAM. RFC 5424 §7.2.4 bounds it at 32 characters,
         *  on the same terms as Software above. */
        const char* SwVersion;
        /** NULL omits the enterpriseId PARAM. RFC 5424 §7.2.2 requires an IANA-registered
         *  private enterprise number; the value is carried as supplied and not validated. */
        const char* EnterpriseId;
        SolidSyslogOriginIpCountFunction GetIpCount; /**< Paired with GetIpAt; either NULL omits the ip PARAMs. */
        SolidSyslogOriginIpAtFunction GetIpAt;
        void* IpContext; /**< Passed to GetIpAt unchanged. */
    };

    /** Create an origin SD source, emitted on every message the owning logger
     *  formats. Never returns NULL: an exhausted pool reports via
     *  SolidSyslog_Error and returns the shared no-op NullSd, so callers need
     *  not null-check the result. */
    struct SolidSyslogStructuredData* SolidSyslogOriginSd_Create(const struct SolidSyslogOriginSdConfig* config);
    /** Release the SD's pool slot. The borrowed config strings are not freed;
     *  the caller owns them. */
    void SolidSyslogOriginSd_Destroy(struct SolidSyslogStructuredData * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGORIGINSD_H */
