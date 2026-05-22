#include "SolidSyslogNullBuffer.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBufferDefinition.h"

static bool NullBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void NullBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);

struct SolidSyslogBuffer* SolidSyslogNullBuffer_Get(void)
{
    static struct SolidSyslogBuffer instance = {.Write = NullBuffer_Write, .Read = NullBuffer_Read};
    return &instance;
}

/* Read returns false ("nothing to deliver") so the Service algorithm
 * sees an empty Buffer and stops draining. */
static bool NullBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead)
{
    (void) base;
    (void) data;
    (void) maxSize;
    *bytesRead = 0;
    return false;
}

/* Write swallows the record — a misconfigured Buffer paired with a
 * caller that doesn't gate Log() must not block or crash. */
static void NullBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size)
{
    (void) base;
    (void) data;
    (void) size;
}
