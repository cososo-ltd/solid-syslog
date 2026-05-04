#include "MutexFake.h"

#include <stddef.h>

#include "SolidSyslogMutexDefinition.h"

enum
{
    SEQUENCE_CAPACITY = 32
};

static struct SolidSyslogMutex instance;
static char                    sequence[SEQUENCE_CAPACITY];
static size_t                  sequenceLength;

static void Lock(struct SolidSyslogMutex* self);
static void Unlock(struct SolidSyslogMutex* self);

static inline void Append(char marker)
{
    if (sequenceLength + 1 < SEQUENCE_CAPACITY)
    {
        sequence[sequenceLength]     = marker;
        sequence[sequenceLength + 1] = '\0';
        sequenceLength += 1;
    }
}

struct SolidSyslogMutex* MutexFake_Create(void)
{
    instance.Lock   = Lock;
    instance.Unlock = Unlock;
    sequence[0]     = '\0';
    sequenceLength  = 0;
    return &instance;
}

void MutexFake_Destroy(void)
{
    instance.Lock   = NULL;
    instance.Unlock = NULL;
    sequence[0]     = '\0';
    sequenceLength  = 0;
}

const char* MutexFake_Sequence(void)
{
    return sequence;
}

static void Lock(struct SolidSyslogMutex* self)
{
    (void) self;
    Append('L');
}

static void Unlock(struct SolidSyslogMutex* self)
{
    (void) self;
    Append('U');
}
