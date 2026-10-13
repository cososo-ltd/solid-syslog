/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the BlockStore. */
#ifndef SOLIDSYSLOGBLOCKSTOREERRORS_H
#define SOLIDSYSLOGBLOCKSTOREERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogBlockStoreErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogBlockStoreErrors
    {
        SOLIDSYSLOG_BLOCK_STORE_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_BLOCK_STORE_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_BLOCK_STORE_ERROR_BLOCK_TOO_SMALL,
        /* Raised under SOLIDSYSLOG_CAT_STORE_WRITE_FAILED. The security policy
           reports why it would not seal. */
        SOLIDSYSLOG_BLOCK_STORE_ERROR_SEAL_FAILED,
        /* Raised under SOLIDSYSLOG_CAT_STORE_FROZEN, each naming the device
           operation that failed: opening the device at create, appending a
           record, starting a new block, disposing of one, marking a record
           sent, reading one back, measuring a block, or marking a record
           discarded. The device beneath, where it reports, says why. */
        SOLIDSYSLOG_BLOCK_STORE_ERROR_OPEN_FAILED,
        SOLIDSYSLOG_BLOCK_STORE_ERROR_APPEND_FAILED,
        SOLIDSYSLOG_BLOCK_STORE_ERROR_ROTATE_FAILED,
        SOLIDSYSLOG_BLOCK_STORE_ERROR_DISPOSE_FAILED,
        /** The record was delivered, but the device still holds it as unsent, so
         *  it is sent once more after the store is created again. */
        SOLIDSYSLOG_BLOCK_STORE_ERROR_MARK_SENT_FAILED,
        SOLIDSYSLOG_BLOCK_STORE_ERROR_READ_FAILED,
        SOLIDSYSLOG_BLOCK_STORE_ERROR_SIZE_FAILED,
        /** A record that failed verification could not be marked discarded, so
         *  the device still holds it as unsent. */
        SOLIDSYSLOG_BLOCK_STORE_ERROR_DISCARD_FAILED,
        /* Raised under SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED, naming why the
           record could not be delivered: the security policy rejected it, or
           it was corrupt. */
        SOLIDSYSLOG_BLOCK_STORE_ERROR_RECORD_REJECTED,
        /** Its header would not parse, or it ran past the end of its block, so
         *  where it ends is unknown and the rest of the block goes with it. */
        SOLIDSYSLOG_BLOCK_STORE_ERROR_RECORD_CORRUPT,
        SOLIDSYSLOG_BLOCK_STORE_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by a BlockStore. A handler matches by
     *  address (event->Source == &SolidSyslogBlockStoreErrorSource), then reads
     *  event->Detail as an enum SolidSyslogBlockStoreErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogBlockStoreErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGBLOCKSTOREERRORS_H */
