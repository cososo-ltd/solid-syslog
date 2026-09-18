/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

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
