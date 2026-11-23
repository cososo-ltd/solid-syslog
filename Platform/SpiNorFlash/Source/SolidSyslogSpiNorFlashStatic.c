/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogSpiNorFlash.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullNorFlash.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSpiNorFlashBus.h"
#include "SolidSyslogSpiNorFlashErrors.h"
#include "SolidSyslogSpiNorFlashPrivate.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogNorFlash;

static bool SpiNorFlash_IsUsableConfig(const struct SolidSyslogSpiNorFlashConfig* config);
static inline bool SpiNorFlash_BusIsComplete(const struct SolidSyslogSpiNorFlashBus* bus);
static inline void SpiNorFlash_ReportBadConfig(enum SolidSyslogSpiNorFlashErrors code);
static struct SolidSyslogNorFlash* SpiNorFlash_CreateInSlot(struct SolidSyslogSpiNorFlashBus* bus);
static inline size_t SpiNorFlash_IndexFromHandle(const struct SolidSyslogNorFlash* base);
static inline void SpiNorFlash_CleanupAtIndex(size_t index, void* context);

static bool SpiNorFlash_InUse[SOLIDSYSLOG_NOR_FLASH_POOL_SIZE];
static struct SolidSyslogSpiNorFlash SpiNorFlash_Pool[SOLIDSYSLOG_NOR_FLASH_POOL_SIZE];
static struct SolidSyslogPoolAllocator SpiNorFlash_Allocator = {SpiNorFlash_InUse, SOLIDSYSLOG_NOR_FLASH_POOL_SIZE};

struct SolidSyslogNorFlash* SolidSyslogSpiNorFlash_Create(const struct SolidSyslogSpiNorFlashConfig* config)
{
    struct SolidSyslogNorFlash* result = SolidSyslogNullNorFlash_Get();

    if (SpiNorFlash_IsUsableConfig(config))
    {
        result = SpiNorFlash_CreateInSlot(config->Bus);
    }

    return result;
}

/* The bus table is the caller's, so it is checked here, once, and nothing
 * that calls through it checks again. WhileBusy alone may be left unfilled,
 * and is looked at where it is called. */
static bool SpiNorFlash_IsUsableConfig(const struct SolidSyslogSpiNorFlashConfig* config)
{
    bool usable = false;

    if (config == NULL)
    {
        SpiNorFlash_ReportBadConfig(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NULL_CONFIG);
    }
    else if (config->Bus == NULL)
    {
        SpiNorFlash_ReportBadConfig(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NULL_BUS);
    }
    else if (!SpiNorFlash_BusIsComplete(config->Bus))
    {
        SpiNorFlash_ReportBadConfig(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_INCOMPLETE);
    }
    else
    {
        usable = true;
    }

    return usable;
}

static inline bool SpiNorFlash_BusIsComplete(const struct SolidSyslogSpiNorFlashBus* bus)
{
    return (bus->Select != NULL) && (bus->Deselect != NULL) && (bus->Write != NULL) && (bus->Read != NULL) &&
           (bus->Milliseconds != NULL);
}

static inline void SpiNorFlash_ReportBadConfig(enum SolidSyslogSpiNorFlashErrors code)
{
    SpiNorFlash_Report(SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY, SOLIDSYSLOG_CAT_BAD_CONFIG, code);
}

static struct SolidSyslogNorFlash* SpiNorFlash_CreateInSlot(struct SolidSyslogSpiNorFlashBus* bus)
{
    struct SolidSyslogNorFlash* result = SolidSyslogNullNorFlash_Get();
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&SpiNorFlash_Allocator);

    if (!SolidSyslogPoolAllocator_IndexIsValid(&SpiNorFlash_Allocator, index))
    {
        SpiNorFlash_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_POOL_EXHAUSTED
        );
    }
    else if (SolidSyslogSpiNorFlash_Initialise(&SpiNorFlash_Pool[index].Base, bus))
    {
        result = &SpiNorFlash_Pool[index].Base;
    }
    else
    {
        /* Give the slot straight back. A part that did not start may start
         * at the next Create, and holding the slot would turn one refusal
         * into pool exhaustion for it. Initialise has already reported
         * why. */
        (void) SolidSyslogPoolAllocator_FreeIfInUse(&SpiNorFlash_Allocator, index, SpiNorFlash_CleanupAtIndex, NULL);
    }

    return result;
}

void SolidSyslogSpiNorFlash_Destroy(struct SolidSyslogNorFlash* base)
{
    size_t index = SpiNorFlash_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&SpiNorFlash_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&SpiNorFlash_Allocator, index, SpiNorFlash_CleanupAtIndex, NULL);
    if (!released)
    {
        SpiNorFlash_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t SpiNorFlash_IndexFromHandle(const struct SolidSyslogNorFlash* base)
{
    size_t result = SOLIDSYSLOG_NOR_FLASH_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_NOR_FLASH_POOL_SIZE; poolIndex++)
    {
        if (base == &SpiNorFlash_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void SpiNorFlash_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogSpiNorFlash_Cleanup(&SpiNorFlash_Pool[index].Base);
}
