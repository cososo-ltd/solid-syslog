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

#include "ExternC.h"

#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslogSdValue;
    struct SolidSyslogStructuredData;

    /** Returns how many origin "ip" PARAMs to emit; GetIpAt is then called for
     *  each index in [0, count). */
    typedef size_t (*SolidSyslogOriginIpCountFunction)(void);
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
        const char* Software; /**< NULL omits the software PARAM. */
        const char* SwVersion; /**< NULL omits the swVersion PARAM. */
        const char* EnterpriseId; /**< NULL omits the enterpriseId PARAM. */
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

EXTERN_C_END

#endif /* SOLIDSYSLOGORIGINSD_H */
