/** @file
 *  A store-and-forward Store backed by a BlockDevice: records are appended to
 *  the current write block, and once a block fills the write rolls to the next,
 *  giving durable retention across a restart. Create resumes from whatever
 *  records are already on the device (scanning the read block, honouring any
 *  per-record integrity trailer from the injected SecurityPolicy) so a reboot
 *  keeps unsent records queued.
 *
 *  MaxBlocks caps retention; DiscardPolicy governs the overflow once every block
 *  is full — Oldest evicts the oldest block to keep accepting writes, Newest
 *  refuses the incoming record, Halt refuses it, latches (IsHalted stops
 *  Service), and fires OnStoreFull once. An optional capacity-threshold function
 *  (queried each Write) drives an edge-triggered OnThresholdCrossed callback for
 *  early back-pressure signalling. Mind the recursion gotcha: under a
 *  PassthroughBuffer, SolidSyslog_Log sends inline, so logging from the
 *  threshold callback re-enters Write — drive the logger from a returning Buffer
 *  or gate the Log instead.
 *
 *  Internally each pool slot composes an inner RecordStore over a BlockSequence,
 *  both drawn from sibling pools; a block too small for one worst-case record is
 *  grown to fit and reported as a WARNING rather than failing Create. */
#ifndef SOLIDSYSLOGBLOCKSTORE_H
#define SOLIDSYSLOGBLOCKSTORE_H

#include <stddef.h>

#include "ExternC.h"

struct SolidSyslogStore;

EXTERN_C_BEGIN

    struct SolidSyslogBlockDevice;
    struct SolidSyslogSecurityPolicy;

    /** What happens once MaxBlocks are full. Oldest keeps accepting writes,
     *  evicting the oldest block to make room; Newest refuses the incoming record;
     *  Halt refuses it, latches (IsHalted, Service stops), and fires OnStoreFull
     *  once on entry. */
    enum SolidSyslogDiscardPolicy
    {
        SOLIDSYSLOG_DISCARD_POLICY_OLDEST,
        SOLIDSYSLOG_DISCARD_POLICY_NEWEST,
        SOLIDSYSLOG_DISCARD_POLICY_HALT
    };

    /** Fired once when a Halt-policy store first fills. */
    typedef void (*SolidSyslogStoreFullCallback)(void* context);

    /** Returns the capacity threshold in bytes; 0 disables. Queried on every Write. */
    typedef size_t (*SolidSyslogStoreThresholdFunction)(void* context);

    /** Edge-triggered: fires once when used-bytes rises from below the threshold to
     *  at-or-above (re-armed when it drops back below). Gotcha under
     *  SolidSyslogPassthroughBuffer, where SolidSyslog_Log sends inline: logging from
     *  this callback recurses into the store's Write. Gate the Log, or drive the
     *  logger from a returning Buffer (e.g. SolidSyslogPosixMessageQueueBuffer). */
    typedef void (*SolidSyslogStoreThresholdCallback)(void* context);

    /** Wiring for SolidSyslogBlockStore_Create. The threshold function and callback
     *  are only active when both are set; each callback's context is passed back
     *  unchanged. Every injected object is caller-owned and must outlive the store;
     *  Destroy releases only the store's own slot. */
    struct SolidSyslogBlockStoreConfig
    {
        struct SolidSyslogBlockDevice* BlockDevice; /**< Required; block store is the sole writer. */
        size_t MaxBlocks; /**< Retention ceiling in blocks; DiscardPolicy governs the overflow. */
        enum SolidSyslogDiscardPolicy DiscardPolicy;
        struct SolidSyslogSecurityPolicy* SecurityPolicy; /**< NULL means no per-record integrity trailer. */
        SolidSyslogStoreFullCallback OnStoreFull;
        void* StoreFullContext;
        SolidSyslogStoreThresholdFunction GetCapacityThreshold;
        SolidSyslogStoreThresholdCallback OnThresholdCrossed;
        void* ThresholdContext; /**< Shared by GetCapacityThreshold and OnThresholdCrossed. */
    };

    /** Create a store from @p config, resuming from any records already on the
     *  device. A NULL config, an exhausted pool, or a failed inner allocation
     *  falls back to the shared NullStore. A block too small for one worst-case
     *  record is grown to fit and reported as a WARNING, not a failure. */
    struct SolidSyslogStore* SolidSyslogBlockStore_Create(const struct SolidSyslogBlockStoreConfig* config);
    /** Release the pool slot; does not destroy the injected BlockDevice or
     *  SecurityPolicy. */
    void SolidSyslogBlockStore_Destroy(struct SolidSyslogStore * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGBLOCKSTORE_H */
