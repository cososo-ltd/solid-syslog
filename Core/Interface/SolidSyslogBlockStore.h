/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A store-and-forward Store backed by a BlockDevice: records are appended to
 *  the current write block, and once a block fills the write rolls to the next,
 *  giving durable retention across a restart. Create resumes from whatever
 *  records are already on the device (scanning the read block, honouring any
 *  per-record integrity trailer from the injected SecurityPolicy) so a reboot
 *  keeps unsent records queued.
 *
 *  MaxBlocks caps retention; the injected discard policy governs the overflow
 *  once every block is full - Oldest evicts the oldest block to keep accepting
 *  writes, Newest and Halt refuse the incoming record, and under Halt the
 *  logger stops at that refusal. An optional capacity-threshold function
 *  (queried each Write) drives an edge-triggered OnThresholdCrossed callback for
 *  early back-pressure signalling. The callback runs on whichever task calls
 *  into the store - normally the one that calls SolidSyslog_Service - and
 *  inside SolidSyslogBlockStore_Create, on the creating task, when the resumed
 *  store is already at or above the threshold.
 *
 *  The first block device operation that fails freezes the store, as
 *  SOLIDSYSLOG_CAT_STORE_FROZEN describes - opening the device at Create
 *  included, so a volume has to be mounted before the store is created. A read
 *  that runs past the end of a block's data, as a power cut mid-append leaves,
 *  is damaged data rather than a failed device, and does not freeze it.
 *
 *  Internally each pool slot composes an inner RecordStore over a BlockSequence,
 *  both drawn from sibling pools; a block too small for one worst-case record is
 *  grown to fit and reported as a WARNING rather than failing Create. */
#ifndef SOLIDSYSLOGBLOCKSTORE_H
#define SOLIDSYSLOGBLOCKSTORE_H

#include <stddef.h>

#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogExternC.h"

struct SolidSyslogStore;

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogBlockDevice;
    struct SolidSyslogSecurityPolicy;

    /** Returns the capacity threshold in bytes; 0 disables. Queried on every Write. */
    typedef size_t (*SolidSyslogStoreThresholdFunction)(void* context);

    /** Edge-triggered: fires once when used-bytes rises from below the threshold to
     *  at-or-above (re-armed when it drops back below). Runs on whichever
     *  task calls into the store - normally the one that calls
     *  SolidSyslog_Service - and inside SolidSyslogBlockStore_Create when the
     *  resumed store is already at or above the threshold. */
    typedef void (*SolidSyslogStoreThresholdCallback)(void* context);

    /** Wiring for SolidSyslogBlockStore_Create. The threshold function and callback
     *  are only active when both are set; each callback's context is passed back
     *  unchanged. Every injected object is caller-owned and must outlive the store;
     *  Destroy releases only the store's own slot. */
    struct SolidSyslogBlockStoreConfig
    {
        struct SolidSyslogBlockDevice* BlockDevice; /**< Required; block store is the sole writer. */
        /** Retention ceiling in blocks, clamped to 2..98; GetDiscardPolicy governs
         *  the overflow. An eviction that fails freezes the store with one block more
         *  than this on the media, and the next start keeps it. */
        size_t MaxBlocks;
        /** Asked, with DiscardPolicyContext, on a write the full store cannot
         *  fit: Oldest evicts the oldest block, Newest and Halt refuse the record.
         *  NULL applies Oldest. Records lost either way are reported as
         *  SOLIDSYSLOG_CAT_STORE_RECORDS_LOST describes. */
        SolidSyslogDiscardPolicyFunction GetDiscardPolicy;
        void* DiscardPolicyContext;
        struct SolidSyslogSecurityPolicy* SecurityPolicy; /**< @optional no per-record integrity trailer when NULL. */
        SolidSyslogStoreThresholdFunction GetCapacityThreshold;
        SolidSyslogStoreThresholdCallback OnThresholdCrossed;
        void* ThresholdContext; /**< Shared by GetCapacityThreshold and OnThresholdCrossed. */
    };

    /** Create a store from @p config, resuming from any records already on the
     *  device. Each of these is reported at CRITICAL and falls back to the
     *  shared NullStore: a NULL config or BlockDevice, an exhausted pool, a
     *  failed inner allocation, and a SecurityPolicy whose trailer is wider than
     *  SOLIDSYSLOG_MAX_INTEGRITY_SIZE, since storing records without it would
     *  leave them unprotected. A block too small for one worst-case record is
     *  grown to fit and reported as a WARNING, not a failure. */
    struct SolidSyslogStore* SolidSyslogBlockStore_Create(const struct SolidSyslogBlockStoreConfig* config);
    /** Release the pool slot; does not destroy the injected BlockDevice or
     *  SecurityPolicy. */
    void SolidSyslogBlockStore_Destroy(struct SolidSyslogStore * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGBLOCKSTORE_H */
