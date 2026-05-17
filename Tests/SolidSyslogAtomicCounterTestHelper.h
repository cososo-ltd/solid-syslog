#ifndef SOLIDSYSLOGATOMICCOUNTERTESTHELPER_H
#define SOLIDSYSLOGATOMICCOUNTERTESTHELPER_H

#include "ExternC.h"
#include "SolidSyslogAtomicCounter.h"

#include <stdint.h>

EXTERN_C_BEGIN

    enum
    {
        TEST_ATOMIC_COUNTER_STORAGE_SIZE = 64
    };

    typedef struct
    {
        uint8_t Bytes[TEST_ATOMIC_COUNTER_STORAGE_SIZE];
    } TestAtomicCounterStorage;

    struct SolidSyslogAtomicCounter* TestAtomicCounter_Create(TestAtomicCounterStorage * storage);
    void TestAtomicCounter_Init(struct SolidSyslogAtomicCounter * base, uint32_t value);
    uint32_t TestAtomicCounter_Increment(struct SolidSyslogAtomicCounter * base);
    void TestAtomicCounter_Destroy(struct SolidSyslogAtomicCounter * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGATOMICCOUNTERTESTHELPER_H */
