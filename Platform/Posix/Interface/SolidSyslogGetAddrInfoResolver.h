#ifndef SOLIDSYSLOGGETADDRINFORESOLVERH
#define SOLIDSYSLOGGETADDRINFORESOLVERH

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    struct SolidSyslogResolver* SolidSyslogGetAddrInfoResolver_Create(void);
    void SolidSyslogGetAddrInfoResolver_Destroy(struct SolidSyslogResolver * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGGETADDRINFORESOLVERH */
