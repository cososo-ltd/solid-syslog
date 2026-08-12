/** @file
 *  A StructuredData source for the RFC 5424 §7.2 "origin" SD-ELEMENT (IANA
 *  SD-ID, so no enterprise-number suffix), emitted on every message the owning
 *  logger formats. It can carry software, swVersion, enterpriseId, and any
 *  number of repeated ip PARAMs. Every field is independently optional: a NULL
 *  string omits its PARAM, and the ip PARAMs appear only when both GetIpCount
 *  and GetIpAt are supplied (GetIpAt is then called once per index). The config
 *  strings are borrowed, not copied, and read at Format time - they must outlive
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
        /** NULL omits the software PARAM. Truncated to this library's 48 decoded-byte
         *  bound; RFC 5424 §7.2.3 allows 48 characters, so multi-byte UTF-8 truncates
         *  earlier, never later. §7.2.3 asks that it name the generating software
         *  rather than repeat APP-NAME. */
        const char* Software;
        /** NULL omits the swVersion PARAM. Truncated to this library's 32 decoded-byte
         *  bound, on the same terms as Software above; RFC 5424 §7.2.4 allows 32
         *  characters. */
        const char* SwVersion;
        /** NULL omits the enterpriseId PARAM. RFC 5424 §7.2.2 asks for your
         *  IANA-registered private enterprise number on its own - "32473", or
         *  "32473.1.2" if you use sub-identifiers below it - rather than the
         *  1.3.6.1.4.1 arc the number already sits under. Registering one is the
         *  caller's to do, and the value's form is not checked here. Until you have
         *  one, RFC 5612 reserves 32473 for examples and testing: use it in a test
         *  build, not on a device that ships. Truncated to 64 bytes, a bound of this
         *  library's rather than the RFC's. */
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
