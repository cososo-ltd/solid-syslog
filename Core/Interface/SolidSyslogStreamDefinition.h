/** @file
 *  The Stream vtable (Open / Send / Read / Close) — the byte-stream (TCP, TLS
 *  over TCP) transport contract an implementor fills in (the Stream extension
 *  point). */
#ifndef SOLIDSYSLOGSTREAMDEFINITION_H
#define SOLIDSYSLOGSTREAMDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogStream.h"
#include "SolidSyslogExternC.h"

struct SolidSyslogAddress;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** The contract a byte-stream transport (TCP, TLS over TCP) fills in; the
     *  library drives it from the servicing pass, so it need not be reentrant.
     *  Each slot's semantics are the corresponding SolidSyslogStream_* function
     *  in SolidSyslogStream.h; an implementer must honour them, notably the
     *  non-blocking bounded behaviour and the close-on-failure lifecycle that
     *  lets the caller reconnect with a bare Open. */
    struct SolidSyslogStream
    {
        bool (*Open)(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
        bool (*Send)(struct SolidSyslogStream* base, const void* buffer, size_t size);
        SolidSyslogSsize (*Read)(struct SolidSyslogStream* base, void* buffer, size_t size);
        void (*Close)(struct SolidSyslogStream* base);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSTREAMDEFINITION_H */
