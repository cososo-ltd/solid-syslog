#ifndef SOLIDSYSLOGCIRCULARBUFFERPRIVATE_H
#define SOLIDSYSLOGCIRCULARBUFFERPRIVATE_H

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogCircularBufferErrors.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogMutex;

struct SolidSyslogCircularBuffer
{
    struct SolidSyslogBuffer Base;
    struct SolidSyslogMutex* Mutex;
    uint8_t* Ring;
    size_t Capacity;
    size_t Head;
    size_t Tail;
    size_t WrapPoint;
};

void CircularBuffer_Initialise(
    struct SolidSyslogBuffer* base,
    struct SolidSyslogMutex* mutex,
    uint8_t* ring,
    size_t ringBytes
);
void CircularBuffer_Cleanup(struct SolidSyslogBuffer* base);

static inline void CircularBuffer_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogCircularBufferErrors code
)
{
    SolidSyslog_Error(severity, &CircularBufferErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGCIRCULARBUFFERPRIVATE_H */
