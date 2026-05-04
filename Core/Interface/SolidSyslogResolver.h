#ifndef SOLIDSYSLOGRESOLVER_H
#define SOLIDSYSLOGRESOLVER_H

#include <stdbool.h>
#include <stdint.h>

#include "ExternC.h"
#include "SolidSyslogTransport.h"

struct SolidSyslogAddress;

EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    bool SolidSyslogResolver_Resolve(struct SolidSyslogResolver * resolver, enum SolidSyslogTransport transport, const char* host, uint16_t port,
                                     struct SolidSyslogAddress* result);

EXTERN_C_END

#endif /* SOLIDSYSLOGRESOLVER_H */
