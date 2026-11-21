/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGSPINORFLASHPRIVATE_H
#define SOLIDSYSLOGSPINORFLASHPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogNorFlashDefinition.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSpiNorFlashErrors.h"
#include "SolidSyslogSpiNorFlashSfdp.h"

struct SolidSyslogSpiNorFlashBus;

struct SolidSyslogSpiNorFlash
{
    struct SolidSyslogNorFlash Base;
    struct SolidSyslogSpiNorFlashBus* Bus;
    /* As the part's SFDP gave it when the flash was created. */
    struct SolidSyslogSpiNorFlashGeometry Geometry;
};

/* bus has been checked: it and its members are filled. Starts the part and
 * learns it. False, and reported, for a part that does not start or that the
 * flash does not drive. */
bool SolidSyslogSpiNorFlash_Initialise(struct SolidSyslogNorFlash* base, struct SolidSyslogSpiNorFlashBus* bus);
void SolidSyslogSpiNorFlash_Cleanup(struct SolidSyslogNorFlash* base);

static inline void SpiNorFlash_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogSpiNorFlashErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogSpiNorFlashErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGSPINORFLASHPRIVATE_H */
