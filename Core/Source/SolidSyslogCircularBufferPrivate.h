/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGCIRCULARBUFFERPRIVATE_H
#define SOLIDSYSLOGCIRCULARBUFFERPRIVATE_H

#include <stdbool.h>
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
    /* A report stands for a run of refused records; the next record kept ends
     * the run. Set under the mutex, before the report is raised, so a handler
     * that logs into the full ring finds the run already reported. */
    bool LossReported;
    /* The last write was refused for lack of room and nothing has been taken
     * out since - Log has outrun Service. Kept under the mutex. */
    bool Full;
};

void SolidSyslogCircularBuffer_Initialise(
    struct SolidSyslogBuffer* base,
    struct SolidSyslogMutex* mutex,
    uint8_t* ring,
    size_t ringBytes
);
void SolidSyslogCircularBuffer_Cleanup(struct SolidSyslogBuffer* base);

static inline void CircularBuffer_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogCircularBufferErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogCircularBufferErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGCIRCULARBUFFERPRIVATE_H */
