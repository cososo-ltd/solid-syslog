/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogSpiNorFlash.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNorFlashCategories.h"
#include "SolidSyslogNorFlashDefinition.h"
#include "SolidSyslogNullNorFlash.h"
#include "SolidSyslogSpiNorFlashBus.h"
#include "SolidSyslogSpiNorFlashErrors.h"
#include "SolidSyslogSpiNorFlashPrivate.h"
#include "SolidSyslogSpiNorFlashSfdp.h"

const struct SolidSyslogErrorSource SolidSyslogSpiNorFlashErrorSource = {
    "SpiNorFlash",
    SolidSyslogSpiNorFlashErrors_Name
};

enum
{
    /* A line with nothing driving it, and one held low. */
    IDLE_LINE = 0xFF,
    LOW_LINE = 0x00,

    READ_STATUS = 0x05,
    READ_SFDP = 0x5A,
    READ_JEDEC_ID = 0x9F,

    STATUS_BUSY = 0x01,
    /* Bits 2 to 6 of the first status register: the block protection, by
     * whichever of the common layouts the part uses. */
    STATUS_PROTECTION = 0x7C,

    /* The command, three bytes of address, and one byte the part ignores. */
    SFDP_REQUEST_LENGTH = 5,
    SFDP_HEADER_ADDRESS = 0,

    /* Long enough for an erase a reset interrupted to finish. */
    START_TIMEOUT_MILLISECONDS = 3000
};

static size_t SpiNorFlash_GetBlockSize(struct SolidSyslogNorFlash* base);
static size_t SpiNorFlash_GetBlockCount(struct SolidSyslogNorFlash* base);

static inline struct SolidSyslogSpiNorFlash* SpiNorFlash_SelfFromBase(struct SolidSyslogNorFlash* base);
static bool SpiNorFlash_WaitUntilReady(struct SolidSyslogSpiNorFlash* self, uint32_t milliseconds);
static inline bool SpiNorFlash_ReadStatus(struct SolidSyslogSpiNorFlash* self, uint8_t* status);
static bool SpiNorFlash_Ask(
    struct SolidSyslogSpiNorFlash* self,
    const uint8_t* request,
    size_t requestCount,
    uint8_t* answer,
    size_t answerCount
);
static inline bool SpiNorFlash_IsBusy(uint8_t status);
static inline bool SpiNorFlash_HasPassed(struct SolidSyslogSpiNorFlash* self, uint32_t since, uint32_t milliseconds);
static inline void SpiNorFlash_ReportIoFailure(enum SolidSyslogSpiNorFlashErrors code);
static bool SpiNorFlash_IsAnswering(struct SolidSyslogSpiNorFlash* self);
static inline bool SpiNorFlash_IsNoMaker(uint8_t maker);
static bool SpiNorFlash_LearnThePart(struct SolidSyslogSpiNorFlash* self);
static bool SpiNorFlash_LocateTheTable(
    struct SolidSyslogSpiNorFlash* self,
    struct SolidSyslogSpiNorFlashSfdpTable* table
);
static inline bool SpiNorFlash_ReadSfdp(
    struct SolidSyslogSpiNorFlash* self,
    uint32_t address,
    uint8_t* buffer,
    size_t count
);
static inline void SpiNorFlash_ReportUnsupported(enum SolidSyslogSpiNorFlashErrors code);
static bool SpiNorFlash_ReadTheTable(
    struct SolidSyslogSpiNorFlash* self,
    const struct SolidSyslogSpiNorFlashSfdpTable* table
);
static bool SpiNorFlash_IsUnprotected(struct SolidSyslogSpiNorFlash* self);

bool SolidSyslogSpiNorFlash_Initialise(struct SolidSyslogNorFlash* base, struct SolidSyslogSpiNorFlashBus* bus)
{
    struct SolidSyslogSpiNorFlash* self = SpiNorFlash_SelfFromBase(base);
    self->Base = *SolidSyslogNullNorFlash_Get();
    self->Base.GetBlockSize = SpiNorFlash_GetBlockSize;
    self->Base.GetBlockCount = SpiNorFlash_GetBlockCount;
    self->Bus = bus;
    self->Geometry.CapacityBytes = 0U;
    self->Geometry.EraseBytes = 0U;

    return SpiNorFlash_WaitUntilReady(self, START_TIMEOUT_MILLISECONDS) && SpiNorFlash_IsAnswering(self) &&
           SpiNorFlash_LearnThePart(self) && SpiNorFlash_IsUnprotected(self);
}

void SolidSyslogSpiNorFlash_Cleanup(struct SolidSyslogNorFlash* base)
{
    /* The abstract base takes the shared NullNorFlash vtable, so use after
     * destroy is a safe no-op. The bus is the caller's and is not touched. */
    *base = *SolidSyslogNullNorFlash_Get();
}

static inline struct SolidSyslogSpiNorFlash* SpiNorFlash_SelfFromBase(struct SolidSyslogNorFlash* base)
{
    return (struct SolidSyslogSpiNorFlash*) base;
}

/* False, and reported, for a part still busy after the time given, one that
 * does not answer, and a bus that fails. */
static bool SpiNorFlash_WaitUntilReady(struct SolidSyslogSpiNorFlash* self, uint32_t milliseconds)
{
    uint32_t since = self->Bus->Milliseconds(self->Bus);
    uint8_t status = IDLE_LINE;
    bool asked = SpiNorFlash_ReadStatus(self, &status);
    bool expired = false;
    bool ready = false;

    while (asked && SpiNorFlash_IsBusy(status) && !expired)
    {
        /* The time is read before the status, so the status is always read
         * once after the timeout has passed. Otherwise a task switched out
         * for longer than the timeout would give up on a part that finished
         * while it was away. */
        expired = SpiNorFlash_HasPassed(self, since, milliseconds);
        asked = SpiNorFlash_ReadStatus(self, &status);
    }

    if (!asked)
    {
        SpiNorFlash_ReportIoFailure(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED);
    }
    else if (status == (uint8_t) IDLE_LINE)
    {
        SpiNorFlash_ReportIoFailure(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_ANSWER);
    }
    else if (SpiNorFlash_IsBusy(status))
    {
        SpiNorFlash_ReportIoFailure(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_TIMED_OUT);
    }
    else
    {
        ready = true;
    }

    return ready;
}

static inline bool SpiNorFlash_ReadStatus(struct SolidSyslogSpiNorFlash* self, uint8_t* status)
{
    const uint8_t request = READ_STATUS;

    return SpiNorFlash_Ask(self, &request, sizeof(request), status, sizeof(*status));
}

/* One command that is answered: the request goes out and the answer comes
 * back within one select. */
static bool SpiNorFlash_Ask(
    struct SolidSyslogSpiNorFlash* self,
    const uint8_t* request,
    size_t requestCount,
    uint8_t* answer,
    size_t answerCount
)
{
    struct SolidSyslogSpiNorFlashBus* bus = self->Bus;

    bus->Select(bus);
    bool answered = bus->Write(bus, request, requestCount) && bus->Read(bus, answer, answerCount);
    bus->Deselect(bus);

    return answered;
}

/* A status of all ones is an undriven line and not a busy part: no part is
 * busy with every protection bit set as well. */
static inline bool SpiNorFlash_IsBusy(uint8_t status)
{
    return (status != (uint8_t) IDLE_LINE) && ((status & (uint8_t) STATUS_BUSY) != 0U);
}

/* Unsigned arithmetic, so it holds when the count wraps. */
static inline bool SpiNorFlash_HasPassed(struct SolidSyslogSpiNorFlash* self, uint32_t since, uint32_t milliseconds)
{
    return (self->Bus->Milliseconds(self->Bus) - since) >= milliseconds;
}

static inline void SpiNorFlash_ReportIoFailure(enum SolidSyslogSpiNorFlashErrors code)
{
    SpiNorFlash_Report(SOLIDSYSLOG_NOR_FLASH_IO_FAILED_SEVERITY, SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED, code);
}

/* A line held low reads as a status of zero, which is a part that is ready.
 * Its maker's code tells the two apart: every maker has one, and it is
 * neither all zeros nor all ones. */
static bool SpiNorFlash_IsAnswering(struct SolidSyslogSpiNorFlash* self)
{
    const uint8_t request = READ_JEDEC_ID;
    uint8_t maker = IDLE_LINE;
    bool answering = false;

    if (!SpiNorFlash_Ask(self, &request, sizeof(request), &maker, sizeof(maker)))
    {
        SpiNorFlash_ReportIoFailure(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED);
    }
    else if (SpiNorFlash_IsNoMaker(maker))
    {
        SpiNorFlash_ReportIoFailure(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_ANSWER);
    }
    else
    {
        answering = true;
    }

    return answering;
}

static inline bool SpiNorFlash_IsNoMaker(uint8_t maker)
{
    return (maker == (uint8_t) IDLE_LINE) || (maker == (uint8_t) LOW_LINE);
}

static bool SpiNorFlash_LearnThePart(struct SolidSyslogSpiNorFlash* self)
{
    struct SolidSyslogSpiNorFlashSfdpTable table = {0U, 0U};

    return SpiNorFlash_LocateTheTable(self, &table) && SpiNorFlash_ReadTheTable(self, &table);
}

static bool SpiNorFlash_LocateTheTable(
    struct SolidSyslogSpiNorFlash* self,
    struct SolidSyslogSpiNorFlashSfdpTable* table
)
{
    uint8_t header[SOLIDSYSLOG_SPI_NOR_FLASH_SFDP_HEADER_BYTES] = {0};
    enum SolidSyslogSpiNorFlashErrors fault = SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_SFDP;
    bool located = false;

    if (!SpiNorFlash_ReadSfdp(self, SFDP_HEADER_ADDRESS, header, sizeof(header)))
    {
        SpiNorFlash_ReportIoFailure(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED);
    }
    else if (!SolidSyslogSpiNorFlashSfdp_Locate(header, table, &fault))
    {
        SpiNorFlash_ReportUnsupported(fault);
    }
    else
    {
        located = true;
    }

    return located;
}

static inline bool SpiNorFlash_ReadSfdp(
    struct SolidSyslogSpiNorFlash* self,
    uint32_t address,
    uint8_t* buffer,
    size_t count
)
{
    /* The last byte is the one the part ignores. It is not all ones: a part
     * can take that as the end of a mode it was never in. */
    const uint8_t request[SFDP_REQUEST_LENGTH] =
        {READ_SFDP, (uint8_t) (address >> 16U), (uint8_t) (address >> 8U), (uint8_t) address, 0x00};

    return SpiNorFlash_Ask(self, request, sizeof(request), buffer, count);
}

static inline void SpiNorFlash_ReportUnsupported(enum SolidSyslogSpiNorFlashErrors code)
{
    SpiNorFlash_Report(SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY, SOLIDSYSLOG_CAT_BAD_CONFIG, code);
}

static bool SpiNorFlash_ReadTheTable(
    struct SolidSyslogSpiNorFlash* self,
    const struct SolidSyslogSpiNorFlashSfdpTable* table
)
{
    uint8_t bytes[SOLIDSYSLOG_SPI_NOR_FLASH_SFDP_TABLE_BYTES] = {0};
    enum SolidSyslogSpiNorFlashErrors fault = SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_SFDP;
    bool read = false;

    if (!SpiNorFlash_ReadSfdp(self, table->Address, bytes, table->Bytes))
    {
        SpiNorFlash_ReportIoFailure(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED);
    }
    else if (!SolidSyslogSpiNorFlashSfdp_ReadGeometry(bytes, &self->Geometry, &fault))
    {
        SpiNorFlash_ReportUnsupported(fault);
    }
    else
    {
        read = true;
    }

    return read;
}

/* The flash never writes a status register, so a part that is protected
 * stays so, and is refused here before a program or an erase can fail on
 * it. */
static bool SpiNorFlash_IsUnprotected(struct SolidSyslogSpiNorFlash* self)
{
    uint8_t status = IDLE_LINE;
    bool unprotected = false;

    if (!SpiNorFlash_ReadStatus(self, &status))
    {
        SpiNorFlash_ReportIoFailure(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED);
    }
    else if ((status & (uint8_t) STATUS_PROTECTION) != 0U)
    {
        SpiNorFlash_ReportUnsupported(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_PART_PROTECTED);
    }
    else
    {
        unprotected = true;
    }

    return unprotected;
}

static size_t SpiNorFlash_GetBlockSize(struct SolidSyslogNorFlash* base)
{
    return SpiNorFlash_SelfFromBase(base)->Geometry.EraseBytes;
}

static size_t SpiNorFlash_GetBlockCount(struct SolidSyslogNorFlash* base)
{
    const struct SolidSyslogSpiNorFlash* self = SpiNorFlash_SelfFromBase(base);

    return self->Geometry.CapacityBytes / self->Geometry.EraseBytes;
}
