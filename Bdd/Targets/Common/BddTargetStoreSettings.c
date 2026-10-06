#include "BddTargetStoreSettings.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "BddTargetMessageSettings.h"

enum
{
    DEFAULT_MAX_BLOCKS = 10,
    DEFAULT_MAX_BLOCK_SIZE = 65536
};

static size_t maxBlocks = DEFAULT_MAX_BLOCKS;
static size_t maxBlockSize = DEFAULT_MAX_BLOCK_SIZE;
static size_t capacityThreshold = 0U;
static enum SolidSyslogDiscardPolicy discardPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
static enum BddTargetSecurityPolicy securityPolicy = BDD_TARGET_SECURITY_POLICY_CRC16;
static bool haltExit = false;
static bool noSd = false;

/* The names the harness sends for an enumerated setting, beside the value
   each stands for. */
struct StoreSettings_Name
{
    const char* Name;
    int Value;
};

static const struct StoreSettings_Name DISCARD_POLICIES[] = {
    {"oldest", SOLIDSYSLOG_DISCARD_POLICY_OLDEST},
    {"newest", SOLIDSYSLOG_DISCARD_POLICY_NEWEST},
    {"halt", SOLIDSYSLOG_DISCARD_POLICY_HALT},
};

static const struct StoreSettings_Name SECURITY_POLICIES[] = {
    {"crc16", BDD_TARGET_SECURITY_POLICY_CRC16},
    {"null", BDD_TARGET_SECURITY_POLICY_NULL},
    {"hmac-sha256", BDD_TARGET_SECURITY_POLICY_HMAC_SHA256},
    {"aes-256-gcm", BDD_TARGET_SECURITY_POLICY_AES_256_GCM},
};

static inline bool StoreSettings_SetSize(const char* value, size_t* setting);
static inline bool StoreSettings_SetFlag(const char* value, bool* setting);
static inline bool StoreSettings_FindName(
    const char* value,
    const struct StoreSettings_Name* names,
    size_t count,
    int* found
);

void BddTargetStoreSettings_Reset(void)
{
    maxBlocks = DEFAULT_MAX_BLOCKS;
    maxBlockSize = DEFAULT_MAX_BLOCK_SIZE;
    capacityThreshold = 0U;
    discardPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    securityPolicy = BDD_TARGET_SECURITY_POLICY_CRC16;
    haltExit = false;
    noSd = false;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bool BddTargetStoreSettings_SetByName(const char* name, const char* value)
{
    bool taken = false;
    int found = 0;

    if (strcmp(name, "max-blocks") == 0)
    {
        taken = StoreSettings_SetSize(value, &maxBlocks);
    }
    else if (strcmp(name, "max-block-size") == 0)
    {
        taken = StoreSettings_SetSize(value, &maxBlockSize);
    }
    else if (strcmp(name, "capacity-threshold") == 0)
    {
        taken = StoreSettings_SetSize(value, &capacityThreshold);
    }
    else if (strcmp(name, "discard-policy") == 0)
    {
        taken = StoreSettings_FindName(
            value,
            DISCARD_POLICIES,
            sizeof(DISCARD_POLICIES) / sizeof(DISCARD_POLICIES[0]),
            &found
        );
        if (taken)
        {
            discardPolicy = (enum SolidSyslogDiscardPolicy) found;
        }
    }
    else if (strcmp(name, "security-policy") == 0)
    {
        taken = StoreSettings_FindName(
            value,
            SECURITY_POLICIES,
            sizeof(SECURITY_POLICIES) / sizeof(SECURITY_POLICIES[0]),
            &found
        );
        if (taken)
        {
            securityPolicy = (enum BddTargetSecurityPolicy) found;
        }
    }
    else if (strcmp(name, "halt-exit") == 0)
    {
        taken = StoreSettings_SetFlag(value, &haltExit);
    }
    else if (strcmp(name, "no-sd") == 0)
    {
        taken = StoreSettings_SetFlag(value, &noSd);
    }
    return taken;
}

static inline bool StoreSettings_SetSize(const char* value, size_t* setting)
{
    unsigned long parsed = 0U;
    bool taken = BddTargetMessageSettings_TryParseNumber(value, &parsed);
    if (taken)
    {
        *setting = (size_t) parsed;
    }
    return taken;
}

/* The harness sends a flag as a number, as it sends every `set` value. */
static inline bool StoreSettings_SetFlag(const char* value, bool* setting)
{
    unsigned long parsed = 0U;
    bool taken = BddTargetMessageSettings_TryParseNumber(value, &parsed);
    if (taken)
    {
        *setting = parsed != 0U;
    }
    return taken;
}

static inline bool StoreSettings_FindName(
    const char* value,
    const struct StoreSettings_Name* names,
    size_t count,
    int* found
)
{
    bool known = false;
    for (size_t index = 0U; (index < count) && !known; index++)
    {
        known = strcmp(value, names[index].Name) == 0;
        if (known)
        {
            *found = names[index].Value;
        }
    }
    return known;
}

size_t BddTargetStoreSettings_MaxBlocks(void)
{
    return maxBlocks;
}

size_t BddTargetStoreSettings_MaxBlockSize(void)
{
    return maxBlockSize;
}

enum SolidSyslogDiscardPolicy BddTargetStoreSettings_DiscardPolicy(void)
{
    return discardPolicy;
}

enum BddTargetSecurityPolicy BddTargetStoreSettings_SecurityPolicy(void)
{
    return securityPolicy;
}

bool BddTargetStoreSettings_HaltExit(void)
{
    return haltExit;
}

bool BddTargetStoreSettings_NoSd(void)
{
    return noSd;
}

size_t BddTargetStoreSettings_GetCapacityThreshold(void* context)
{
    (void) context;
    return capacityThreshold;
}
