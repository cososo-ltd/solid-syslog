#ifndef SOLIDSYSLOGCIRCULARBUFFER_H
#define SOLIDSYSLOGCIRCULARBUFFER_H

#include "ExternC.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslog.h"

EXTERN_C_BEGIN

    struct SolidSyslogBuffer;

    typedef size_t SolidSyslogCircularBufferStorage;

    enum
    {
        SOLIDSYSLOG_CIRCULARBUFFER_OVERHEAD     = 6,
        SOLIDSYSLOG_CIRCULARBUFFER_HEADER_BYTES = sizeof(uint16_t)
    };

/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- cannot compute array size as a constexpr in C */
#define SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE(maxMessages)                                                                                              \
    (SOLIDSYSLOG_CIRCULARBUFFER_OVERHEAD                                                                                                                  \
     + (((maxMessages) * (SOLIDSYSLOG_MAX_MESSAGE_SIZE + SOLIDSYSLOG_CIRCULARBUFFER_HEADER_BYTES) + sizeof(SolidSyslogCircularBufferStorage) - 1)         \
        / sizeof(SolidSyslogCircularBufferStorage)))

    struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(SolidSyslogCircularBufferStorage * storage, size_t maxMessages);
    void                      SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer * buffer);

EXTERN_C_END

#endif /* SOLIDSYSLOGCIRCULARBUFFER_H */
