#include "SolidSyslogOpenSslAesGcmPolicy.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogOpenSslAesGcmPolicyErrors.h"
#include "SolidSyslogOpenSslAesGcmPolicyPrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"

static inline bool OpenSslAesGcmPolicy_ConfigIsValid(const struct SolidSyslogOpenSslAesGcmPolicyConfig* config);
static inline size_t OpenSslAesGcmPolicy_IndexFromHandle(const struct SolidSyslogSecurityPolicy* base);
static inline void OpenSslAesGcmPolicy_CleanupAtIndex(size_t index, void* context);

static bool OpenSslAesGcmPolicy_InUse[SOLIDSYSLOG_AES_GCM_POLICY_POOL_SIZE];
static struct SolidSyslogOpenSslAesGcmPolicy OpenSslAesGcmPolicy_Pool[SOLIDSYSLOG_AES_GCM_POLICY_POOL_SIZE];
static struct SolidSyslogPoolAllocator OpenSslAesGcmPolicy_Allocator = {
    OpenSslAesGcmPolicy_InUse,
    SOLIDSYSLOG_AES_GCM_POLICY_POOL_SIZE
};

struct SolidSyslogSecurityPolicy* SolidSyslogOpenSslAesGcmPolicy_Create(
    const struct SolidSyslogOpenSslAesGcmPolicyConfig* config
)
{
    struct SolidSyslogSecurityPolicy* handle = SolidSyslogNullSecurityPolicy_Get();
    if (OpenSslAesGcmPolicy_ConfigIsValid(config))
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&OpenSslAesGcmPolicy_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&OpenSslAesGcmPolicy_Allocator, index) == true)
        {
            OpenSslAesGcmPolicy_Initialise(&OpenSslAesGcmPolicy_Pool[index].Base, config);
            handle = &OpenSslAesGcmPolicy_Pool[index].Base;
        }
        else
        {
            OpenSslAesGcmPolicy_Report(
                SOLIDSYSLOG_SEVERITY_ERROR,
                SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
                OPENSSLAESGCMPOLICY_ERROR_POOL_EXHAUSTED
            );
        }
    }
    else
    {
        OpenSslAesGcmPolicy_Report(
            SOLIDSYSLOG_SEVERITY_ERROR,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            OPENSSLAESGCMPOLICY_ERROR_BAD_CONFIG
        );
    }
    return handle;
}

void SolidSyslogOpenSslAesGcmPolicy_Destroy(struct SolidSyslogSecurityPolicy* base)
{
    size_t index = OpenSslAesGcmPolicy_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&OpenSslAesGcmPolicy_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &OpenSslAesGcmPolicy_Allocator,
                        index,
                        OpenSslAesGcmPolicy_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        OpenSslAesGcmPolicy_Report(
            SOLIDSYSLOG_SEVERITY_WARNING,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            OPENSSLAESGCMPOLICY_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline bool OpenSslAesGcmPolicy_ConfigIsValid(const struct SolidSyslogOpenSslAesGcmPolicyConfig* config)
{
    return (config != NULL) && (config->GetKey != NULL);
}

static inline size_t OpenSslAesGcmPolicy_IndexFromHandle(const struct SolidSyslogSecurityPolicy* base)
{
    size_t result = SOLIDSYSLOG_AES_GCM_POLICY_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_AES_GCM_POLICY_POOL_SIZE; poolIndex++)
    {
        if (base == &OpenSslAesGcmPolicy_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void OpenSslAesGcmPolicy_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    OpenSslAesGcmPolicy_Cleanup(&OpenSslAesGcmPolicy_Pool[index].Base);
}
