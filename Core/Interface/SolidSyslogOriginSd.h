#ifndef SOLIDSYSLOGORIGINSD_H
#define SOLIDSYSLOGORIGINSD_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogStructuredData;

    struct SolidSyslogOriginSdConfig
    {
        const char* software;
        const char* swVersion;
    };

    struct SolidSyslogStructuredData* SolidSyslogOriginSd_Create(const struct SolidSyslogOriginSdConfig* config);
    void                              SolidSyslogOriginSd_Destroy(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGORIGINSD_H */
