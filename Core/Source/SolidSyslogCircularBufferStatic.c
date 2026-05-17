#include "SolidSyslogCircularBuffer.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogCircularBufferPrivate.h"

struct SolidSyslogMutex;

static struct SolidSyslogCircularBuffer Instance;

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
    struct SolidSyslogMutex* mutex,
    uint8_t*                 ring,
    size_t                   ringBytes
)
{
    CircularBuffer_Initialise(&Instance, mutex, ring, ringBytes);
    return &Instance.Base;
}

/* cppcheck-suppress constParameter -- public API is non-const (mutating Destroy); next commit's pool ownership check exercises base for identity comparison and Cleanup mutates through it. */
void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer* base)
{
    if (base == &Instance.Base)
    {
        CircularBuffer_Cleanup(&Instance);
    }
}
