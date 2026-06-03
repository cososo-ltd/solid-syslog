#ifndef SOLIDSYSLOGPOSIXMUTEXPRIVATE_H
#define SOLIDSYSLOGPOSIXMUTEXPRIVATE_H

#include <stdint.h>

#include <pthread.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogPosixMutexErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogPosixMutex
{
    struct SolidSyslogMutex Base;
    pthread_mutex_t Mutex;
};

void PosixMutex_Initialise(struct SolidSyslogMutex* base);
void PosixMutex_Cleanup(struct SolidSyslogMutex* base);

static inline void PosixMutex_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPosixMutexErrors code
)
{
    SolidSyslog_Error(severity, &PosixMutexErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPOSIXMUTEXPRIVATE_H */
