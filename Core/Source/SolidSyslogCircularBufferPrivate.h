#ifndef SOLIDSYSLOGCIRCULARBUFFERPRIVATE_H
#define SOLIDSYSLOGCIRCULARBUFFERPRIVATE_H

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBufferDefinition.h"

struct SolidSyslogMutex;

struct SolidSyslogCircularBuffer
{
    struct SolidSyslogBuffer Base;
    struct SolidSyslogMutex* Mutex;
    uint8_t*                 Ring;
    size_t                   Capacity;
    size_t                   Head;
    size_t                   Tail;
    size_t                   WrapPoint;
};

void CircularBuffer_Initialise(
    struct SolidSyslogCircularBuffer* self,
    struct SolidSyslogMutex*          mutex,
    uint8_t*                          ring,
    size_t                            ringBytes
);
void CircularBuffer_Cleanup(struct SolidSyslogCircularBuffer* self);

#endif /* SOLIDSYSLOGCIRCULARBUFFERPRIVATE_H */
