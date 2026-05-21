#include "SolidSyslogMbedTlsStream.h"

#include "SolidSyslogMbedTlsStreamPrivate.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"

static inline struct SolidSyslogMbedTlsStream* MbedTlsStream_SelfFromBase(struct SolidSyslogStream* base);

void MbedTlsStream_Initialise(struct SolidSyslogStream* base, const struct SolidSyslogMbedTlsStreamConfig* config)
{
    /* Slice 1 (plumbing): no mbedTLS API calls yet. Wire the vtable to the
     * shared NullStream so any caller exercising the handle gets safe no-ops
     * until slice 2 lands the real Open/Send/Read/Close path. */
    struct SolidSyslogMbedTlsStream* self = MbedTlsStream_SelfFromBase(base);
    self->Base = *SolidSyslogNullStream_Get();
    self->Config = *config;
}

void MbedTlsStream_Cleanup(struct SolidSyslogStream* base)
{
    /* Overwrite the abstract base with the shared NullStream vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullStream_Get();
}

static inline struct SolidSyslogMbedTlsStream* MbedTlsStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogMbedTlsStream*) base;
}
