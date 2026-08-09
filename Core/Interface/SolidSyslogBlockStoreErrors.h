/** @file
 *  Error codes and Source identity for the BlockStore. */
#ifndef SOLIDSYSLOGBLOCKSTOREERRORS_H
#define SOLIDSYSLOGBLOCKSTOREERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is BlockStoreErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogBlockStoreErrors
    {
        SOLIDSYSLOG_BLOCK_STORE_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_BLOCK_STORE_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_BLOCK_STORE_ERROR_BLOCK_TOO_SMALL,
        SOLIDSYSLOG_BLOCK_STORE_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by a BlockStore. A handler matches by
     *  address (event->Source == &BlockStoreErrorSource), then reads
     *  event->Detail as an enum SolidSyslogBlockStoreErrors. */
    extern const struct SolidSyslogErrorSource BlockStoreErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGBLOCKSTOREERRORS_H */
