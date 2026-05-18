#ifndef SOLIDSYSLOGORIGINSD_H
#define SOLIDSYSLOGORIGINSD_H

#include "ExternC.h"

#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslogFormatter;
    struct SolidSyslogStructuredData;

    typedef size_t (*SolidSyslogOriginIpCountFunction)(void); // NOLINT(modernize-redundant-void-arg) -- C idiom
    typedef void (*SolidSyslogOriginIpAtFunction)(struct SolidSyslogFormatter* formatter, size_t index);

    struct SolidSyslogOriginSdConfig
    {
        const char* Software;
        const char* SwVersion;
        const char* EnterpriseId;
        SolidSyslogOriginIpCountFunction GetIpCount;
        SolidSyslogOriginIpAtFunction GetIpAt;
    };

    struct SolidSyslogStructuredData* SolidSyslogOriginSd_Create(const struct SolidSyslogOriginSdConfig* config);
    void SolidSyslogOriginSd_Destroy(struct SolidSyslogStructuredData * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGORIGINSD_H */
