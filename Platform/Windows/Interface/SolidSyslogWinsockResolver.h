#ifndef SOLIDSYSLOGWINSOCKRESOLVERH
#define SOLIDSYSLOGWINSOCKRESOLVERH

#include "SolidSyslogResolver.h"

EXTERN_C_BEGIN

    /* Precondition: caller has invoked WSAStartup() before using the resolver,
       and will call WSACleanup() on shutdown. The library does not manage
       Winsock lifecycle. */
    struct SolidSyslogResolver* SolidSyslogWinsockResolver_Create(void);
    void SolidSyslogWinsockResolver_Destroy(struct SolidSyslogResolver * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKRESOLVERH */
