#ifndef SOLIDSYSLOGFREERTOSSTATICRESOLVER_H
#define SOLIDSYSLOGFREERTOSSTATICRESOLVER_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    enum
    {
        SOLIDSYSLOG_FREERTOSSTATICRESOLVER_SIZE = sizeof(intptr_t) * 4U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_FREERTOSSTATICRESOLVER_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogFreeRtosStaticResolverStorage;

    struct SolidSyslogResolver* SolidSyslogFreeRtosStaticResolver_Create(
        SolidSyslogFreeRtosStaticResolverStorage * storage,
        const uint8_t ipv4Octets[4]
    );
    void SolidSyslogFreeRtosStaticResolver_Destroy(struct SolidSyslogResolver * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSSTATICRESOLVER_H */
