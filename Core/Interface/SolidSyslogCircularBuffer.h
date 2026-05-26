#ifndef SOLIDSYSLOGCIRCULARBUFFER_H
#define SOLIDSYSLOGCIRCULARBUFFER_H

#include "ExternC.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogTunables.h"

EXTERN_C_BEGIN

    struct SolidSyslogBuffer;
    struct SolidSyslogMutex;

    enum
    {
        SOLIDSYSLOG_CIRCULAR_BUFFER_HEADER_BYTES = sizeof(uint16_t)
    };

#define SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(maxMessages) \
    ((maxMessages) * (SOLIDSYSLOG_MAX_MESSAGE_SIZE + SOLIDSYSLOG_CIRCULAR_BUFFER_HEADER_BYTES))

    struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
        struct SolidSyslogMutex * mutex,
        uint8_t* ring,
        size_t ringBytes
    );
    void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGCIRCULARBUFFER_H */
