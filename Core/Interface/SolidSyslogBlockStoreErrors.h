#ifndef SOLIDSYSLOGBLOCKSTOREERRORS_H
#define SOLIDSYSLOGBLOCKSTOREERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    enum SolidSyslogBlockStoreErrors
    {
        BLOCKSTORE_ERROR_POOL_EXHAUSTED,
        BLOCKSTORE_ERROR_UNKNOWN_DESTROY,
        BLOCKSTORE_ERROR_BLOCK_TOO_SMALL,
        BLOCKSTORE_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource BlockStoreErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGBLOCKSTOREERRORS_H */
