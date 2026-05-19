#ifndef SOLIDSYSLOGBLOCKSTORE_H
#define SOLIDSYSLOGBLOCKSTORE_H

#include <stddef.h>

#include "SolidSyslogSecurityPolicyDefinition.h"
#include "ExternC.h"

struct SolidSyslogStore;

EXTERN_C_BEGIN

    struct SolidSyslogBlockDevice;

    enum SolidSyslogDiscardPolicy
    {
        SOLIDSYSLOG_DISCARD_POLICY_OLDEST,
        SOLIDSYSLOG_DISCARD_POLICY_NEWEST,
        SOLIDSYSLOG_DISCARD_POLICY_HALT
    };

    typedef void (*SolidSyslogStoreFullCallback)(void* context);

    /* Returns the capacity-threshold in bytes; 0 disables. Queried on every Write. */
    typedef size_t (*SolidSyslogStoreThresholdFunction)(void* context);

    /* Edge-triggered: fires once when used-bytes transitions from below threshold to at-or-above.
     * PassthroughBuffer note: SolidSyslog_Log is synchronous under SolidSyslogPassthroughBuffer, so calling
     * SolidSyslog_Log from this callback will recurse into Store_Write. Either gate the Log,
     * or use SolidSyslogPosixMessageQueueBuffer (which returns immediately). */
    typedef void (*SolidSyslogStoreThresholdCallback)(void* context);

    struct SolidSyslogBlockStoreConfig
    {
        /* Required. Caller-owned: must outlive the BlockStore. SolidSyslogBlockStore_Destroy
         * does NOT destroy the block device — that is the integrator's responsibility. */
        struct SolidSyslogBlockDevice* BlockDevice;
        size_t MaxBlockSize;
        size_t MaxBlocks;
        enum SolidSyslogDiscardPolicy DiscardPolicy;
        struct SolidSyslogSecurityPolicy* SecurityPolicy;
        SolidSyslogStoreFullCallback OnStoreFull;
        void* StoreFullContext;
        SolidSyslogStoreThresholdFunction GetCapacityThreshold;
        SolidSyslogStoreThresholdCallback OnThresholdCrossed;
        void* ThresholdContext;
    };

    struct SolidSyslogStore* SolidSyslogBlockStore_Create(const struct SolidSyslogBlockStoreConfig* config);
    void SolidSyslogBlockStore_Destroy(struct SolidSyslogStore * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGBLOCKSTORE_H */
