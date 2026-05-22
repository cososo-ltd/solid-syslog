#ifndef SOLIDSYSLOGFREERTOSRESOLVER_H
#define SOLIDSYSLOGFREERTOSRESOLVER_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    struct SolidSyslogResolver* SolidSyslogFreeRtosResolver_Create(const uint8_t ipv4Octets[4]);
    void SolidSyslogFreeRtosResolver_Destroy(struct SolidSyslogResolver * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSRESOLVER_H */
