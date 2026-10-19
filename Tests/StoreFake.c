#include "StoreFake.h"

#include <string.h>
#include <stdbool.h>

#include "SolidSyslogStoreDefinition.h"
#include "MinSize.h"

enum
{
    STOREFAKE_MAX_MESSAGES = 8,
    STOREFAKE_MAX_MESSAGE_SIZE = 1024
};

static enum SolidSyslogStoreWriteResult Write(struct SolidSyslogStore* self, const void* data, size_t size);
static bool ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead);
static void MarkSent(struct SolidSyslogStore* self);
static bool HasUnsent(struct SolidSyslogStore* self);
static bool IsTransient(struct SolidSyslogStore* self);

struct StoreFake
{
    struct SolidSyslogStore base;
    char entries[STOREFAKE_MAX_MESSAGES][STOREFAKE_MAX_MESSAGE_SIZE];
    size_t sizes[STOREFAKE_MAX_MESSAGES];
    size_t count;
    int writeCount;
    bool failNextWrite;
    bool loseNextWrite;
    bool failNextRead;
};

static struct StoreFake instance;

struct SolidSyslogStore* StoreFake_Create(void)
{
    instance = (struct StoreFake) {0};
    instance.base.Write = Write;
    instance.base.ReadNextUnsent = ReadNextUnsent;
    instance.base.MarkSent = MarkSent;
    instance.base.HasUnsent = HasUnsent;
    instance.base.IsTransient = IsTransient;
    return &instance.base;
}

void StoreFake_Destroy(void)
{
    instance = (struct StoreFake) {0};
}

void StoreFake_FailNextWrite(void)
{
    instance.failNextWrite = true;
}

void StoreFake_LoseNextWrite(void)
{
    instance.loseNextWrite = true;
}

void StoreFake_FailNextRead(void)
{
    instance.failNextRead = true;
}

int StoreFake_WriteCallCount(struct SolidSyslogStore* store)
{
    return ((struct StoreFake*) store)->writeCount;
}

static enum SolidSyslogStoreWriteResult Write(struct SolidSyslogStore* self, const void* data, size_t size)
{
    struct StoreFake* fake = (struct StoreFake*) self;

    if (fake->failNextWrite)
    {
        fake->failNextWrite = false;
        return SOLIDSYSLOG_STORE_WRITE_NOT_RETAINED;
    }

    if (fake->loseNextWrite)
    {
        fake->loseNextWrite = false;
        return SOLIDSYSLOG_STORE_WRITE_LOST;
    }

    /* Queue full mirrors a real store refusing a record: not retained, and
     * writeCount untouched, so HasUnsent() and the test-side counters stay
     * consistent with what was actually retained. */
    if (fake->count >= STOREFAKE_MAX_MESSAGES)
    {
        return SOLIDSYSLOG_STORE_WRITE_NOT_RETAINED;
    }

    size_t copySize = MinSize(size, STOREFAKE_MAX_MESSAGE_SIZE);
    memcpy(fake->entries[fake->count], data, copySize);
    fake->sizes[fake->count] = copySize;
    fake->count++;
    fake->writeCount++;
    return SOLIDSYSLOG_STORE_WRITE_KEPT;
}

static bool ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct StoreFake* fake = (struct StoreFake*) self;

    if (fake->failNextRead)
    {
        fake->failNextRead = false;
        *bytesRead = 0;
        return false;
    }

    if (fake->count == 0)
    {
        return false;
    }

    size_t copySize = MinSize(fake->sizes[0], maxSize);
    memcpy(data, fake->entries[0], copySize);
    *bytesRead = copySize;
    return true;
}

static void MarkSent(struct SolidSyslogStore* self)
{
    struct StoreFake* fake = (struct StoreFake*) self;

    if (fake->count == 0)
    {
        return;
    }

    for (size_t i = 1; i < fake->count; i++)
    {
        memcpy(fake->entries[i - 1], fake->entries[i], fake->sizes[i]);
        fake->sizes[i - 1] = fake->sizes[i];
    }
    fake->count--;
}

static bool HasUnsent(struct SolidSyslogStore* self)
{
    struct StoreFake* fake = (struct StoreFake*) self;
    return fake->count > 0;
}

/* StoreFake models a real store - a Write rejection is a policy decision,
 * not a "please try elsewhere" signal. Service must not bypass to the
 * sender on rejection. */
static bool IsTransient(struct SolidSyslogStore* self)
{
    (void) self;
    return false;
}
