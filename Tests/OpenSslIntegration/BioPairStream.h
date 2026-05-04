#ifndef BIOPAIRSTREAM_H
#define BIOPAIRSTREAM_H

#include <openssl/types.h>

#include "ExternC.h"

struct SolidSyslogStream;

EXTERN_C_BEGIN

    typedef void (*BioPairStreamPumpFunction)(void* context);

    struct SolidSyslogStream* BioPairStream_Create(BIO * bio);
    void                      BioPairStream_Destroy(struct SolidSyslogStream * self);
    void                      BioPairStream_SetPump(struct SolidSyslogStream * self, BioPairStreamPumpFunction pump, void* context);

EXTERN_C_END

#endif /* BIOPAIRSTREAM_H */
