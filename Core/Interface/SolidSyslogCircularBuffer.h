#ifndef SOLIDSYSLOGCIRCULARBUFFER_H
#define SOLIDSYSLOGCIRCULARBUFFER_H

#include "ExternC.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogTunables.h"

EXTERN_C_BEGIN

    struct SolidSyslogBuffer;
    struct SolidSyslogMutex;

    typedef size_t SolidSyslogCircularBufferStorage;

    enum
    {
        SOLIDSYSLOG_CIRCULARBUFFER_OVERHEAD = 7,
        SOLIDSYSLOG_CIRCULARBUFFER_HEADER_BYTES = sizeof(uint16_t)
    };

/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- cannot compute array size as a constexpr in C */
#define SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE_BYTES(ringBytes) \
    (SOLIDSYSLOG_CIRCULARBUFFER_OVERHEAD +                       \
     (((ringBytes) + sizeof(SolidSyslogCircularBufferStorage) - 1U) / sizeof(SolidSyslogCircularBufferStorage)))

/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- cannot compute array size as a constexpr in C */
#define SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE(maxMessages)                                              \
    SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE_BYTES(                                                        \
        (size_t) (maxMessages) * (SOLIDSYSLOG_MAX_MESSAGE_SIZE + SOLIDSYSLOG_CIRCULARBUFFER_HEADER_BYTES) \
    )

    struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
        SolidSyslogCircularBufferStorage * storage,
        size_t storageBytes,
        struct SolidSyslogMutex* mutex
    );
    void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer * buffer);

EXTERN_C_END

#endif /* SOLIDSYSLOGCIRCULARBUFFER_H */
