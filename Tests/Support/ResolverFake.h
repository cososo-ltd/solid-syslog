#ifndef RESOLVERFAKE_H
#define RESOLVERFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    /* A Resolver that answers true to every host and writes nothing into the
     * address: a fixed destination the stream fake treats as a token. One
     * instance; its count runs from the last Reset. */
    struct SolidSyslogResolver* ResolverFake_Get(void);
    void ResolverFake_Reset(void);
    int ResolverFake_ResolveCallCount(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* RESOLVERFAKE_H */
