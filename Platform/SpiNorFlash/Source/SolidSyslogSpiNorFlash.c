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

    PAGE_PROGRAM = 0x02,
    READ_DATA = 0x03,
    READ_STATUS = 0x05,
    WRITE_ENABLE = 0x06,
    READ_SFDP = 0x5A,
    READ_JEDEC_ID = 0x9F,

    STATUS_BUSY = 0x01,
    STATUS_WRITE_ENABLED = 0x02,
    /* Bits 2 to 6 of the first status register: the block protection, by
     * whichever of the common layouts the part uses. */
    STATUS_PROTECTION = 0x7C,

    /* The command, three bytes of address, and one byte the part ignores. */
    SFDP_REQUEST_LENGTH = 5,
    SFDP_HEADER_ADDRESS = 0,

    /* A command and the three bytes of address after it. */
    COMMAND_AND_ADDRESS_LENGTH = 4,

    /* Long enough for an erase a reset interrupted to finish. */
    START_TIMEOUT_MILLISECONDS = 3000,
    /* For asking whether the part is ready now. */
    NO_WAIT = 0
};

/* vtable - forward-declared because SolidSyslogSpiNorFlash_Initialise wires them before their definitions */
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool SpiNorFlash_Read(
    struct SolidSyslogNorFlash* base,
    size_t blockIndex,
    size_t offset,
    void* buffer,
    size_t count
);
static bool SpiNorFlash_Program(
    struct SolidSyslogNorFlash* base,
    size_t blockIndex,
    size_t offset,
    const void* buffer,
    size_t count
);
// NOLINTEND(bugprone-easily-swappable-parameters)
static bool SpiNorFlash_Erase(struct SolidSyslogNorFlash* base, size_t blockIndex);
static size_t SpiNorFlash_GetBlockSize(struct SolidSyslogNorFlash* base);
static size_t SpiNorFlash_GetBlockCount(struct SolidSyslogNorFlash* base);

static inline struct SolidSyslogSpiNorFlash* SpiNorFlash_SelfFromBase(struct SolidSyslogNorFlash* base);
static bool SpiNorFlash_WaitUntilReady(struct SolidSyslogSpiNorFlash* self, uint32_t milliseconds);
static inline bool SpiNorFlash_ReadStatus(struct SolidSyslogSpiNorFlash* self, uint8_t* status);
static bool SpiNorFlash_Ask(
    struct SolidSyslogSpiNorFlash* self,
    const uint8_t* request,
    size_t requestCount,
    void* answer,
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
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- a block, an offset in it and a count; distinct semantics
static bool SpiNorFlash_CheckSpan(
    const struct SolidSyslogSpiNorFlash* self,
    size_t blockIndex,
    size_t offset,
    size_t count
);
static inline bool SpiNorFlash_IsWithinABlock(
    const struct SolidSyslogSpiNorFlash* self,
    size_t blockIndex,
    size_t offset,
    size_t count
);
static inline uint32_t SpiNorFlash_AddressOf(
    const struct SolidSyslogSpiNorFlash* self,
    size_t blockIndex,
    size_t offset
);
// NOLINTEND(bugprone-easily-swappable-parameters)
static inline void SpiNorFlash_SetCommand(uint8_t* request, uint8_t command, uint32_t address);
static inline bool SpiNorFlash_BusHeld(bool held);
static inline size_t SpiNorFlash_BytesToProgramInPage(
    const struct SolidSyslogSpiNorFlash* self,
    uint32_t address,
    size_t remaining
);
static bool SpiNorFlash_ProgramInPage(
    struct SolidSyslogSpiNorFlash* self,
    uint32_t address,
    const uint8_t* bytes,
    size_t count
);
static bool SpiNorFlash_EnableWriting(struct SolidSyslogSpiNorFlash* self);
static bool SpiNorFlash_Send(struct SolidSyslogSpiNorFlash* self, const uint8_t* command, size_t count);
static bool SpiNorFlash_SendWithData(
    struct SolidSyslogSpiNorFlash* self,
    const uint8_t* command,
    size_t commandCount,
    const uint8_t* data,
    size_t dataCount
);

bool SolidSyslogSpiNorFlash_Initialise(struct SolidSyslogNorFlash* base, struct SolidSyslogSpiNorFlashBus* bus)
{
    struct SolidSyslogSpiNorFlash* self = SpiNorFlash_SelfFromBase(base);
    self->Base.Read = SpiNorFlash_Read;
    self->Base.Program = SpiNorFlash_Program;
    self->Base.Erase = SpiNorFlash_Erase;
    self->Base.GetBlockSize = SpiNorFlash_GetBlockSize;
    self->Base.GetBlockCount = SpiNorFlash_GetBlockCount;
    self->Bus = bus;

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
    void* answer,
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
    else if (!SolidSyslogSpiNorFlashSfdp_ReadGeometry(bytes, table, &self->Geometry, &fault))
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

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool SpiNorFlash_Read(
    struct SolidSyslogNorFlash* base,
    size_t blockIndex,
    size_t offset,
    void* buffer,
    size_t count
)
{
    struct SolidSyslogSpiNorFlash* self = SpiNorFlash_SelfFromBase(base);
    bool read = false;

    /* A part still busy with something that outlasted its time answers only
     * a status read, and a data read would appear to succeed with nothing
     * sent back. So it is asked first. */
    if (SpiNorFlash_CheckSpan(self, blockIndex, offset, count) && SpiNorFlash_WaitUntilReady(self, NO_WAIT))
    {
        uint8_t request[COMMAND_AND_ADDRESS_LENGTH] = {0};
        SpiNorFlash_SetCommand(request, READ_DATA, SpiNorFlash_AddressOf(self, blockIndex, offset));
        read = SpiNorFlash_BusHeld(SpiNorFlash_Ask(self, request, sizeof(request), buffer, count));
    }

    return read;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- a block, an offset in it and a count; distinct semantics
/* False, and reported, for a span that is not all within one block of the
 * part. An address past the end of a part wraps to its start, so nothing is
 * sent for one. */
static bool SpiNorFlash_CheckSpan(
    const struct SolidSyslogSpiNorFlash* self,
    size_t blockIndex,
    size_t offset,
    size_t count
)
{
    bool within = SpiNorFlash_IsWithinABlock(self, blockIndex, offset, count);

    if (!within)
    {
        SpiNorFlash_Report(
            SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_ARGUMENT,
            SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_OUT_OF_RANGE
        );
    }

    return within;
}

/* The subtraction keeps `offset + count` from wrapping past the comparison. */
static inline bool SpiNorFlash_IsWithinABlock(
    const struct SolidSyslogSpiNorFlash* self,
    size_t blockIndex,
    size_t offset,
    size_t count
)
{
    size_t blockSize = (size_t) self->Geometry.EraseBytes;
    size_t blockCount = (size_t) (self->Geometry.CapacityBytes / self->Geometry.EraseBytes);

    return (blockIndex < blockCount) && (count <= blockSize) && (offset <= (blockSize - count));
}

/* For a span SpiNorFlash_CheckSpan has passed, so it is within the part. */
static inline uint32_t SpiNorFlash_AddressOf(
    const struct SolidSyslogSpiNorFlash* self,
    size_t blockIndex,
    size_t offset
)
{
    return ((uint32_t) blockIndex * self->Geometry.EraseBytes) + (uint32_t) offset;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

/* A command that takes an address, most significant byte first. */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- a command byte and an address; distinct semantics
static inline void SpiNorFlash_SetCommand(uint8_t* request, uint8_t command, uint32_t address)
{
    request[0] = command;
    request[1] = (uint8_t) (address >> 16U);
    request[2] = (uint8_t) (address >> 8U);
    request[3] = (uint8_t) address;
}

/* Reports a call of the bus that failed, and passes its answer on. */
static inline bool SpiNorFlash_BusHeld(bool held)
{
    if (!held)
    {
        SpiNorFlash_ReportIoFailure(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED);
    }

    return held;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool SpiNorFlash_Program(
    struct SolidSyslogNorFlash* base,
    size_t blockIndex,
    size_t offset,
    const void* buffer,
    size_t count
)
{
    struct SolidSyslogSpiNorFlash* self = SpiNorFlash_SelfFromBase(base);
    const uint8_t* bytes = (const uint8_t*) buffer;
    size_t done = 0U;
    /* A part still busy keeps writing enabled, so it would seem to take a
     * program it ignores. So it is asked first whether it is ready. */
    bool programmed =
        SpiNorFlash_CheckSpan(self, blockIndex, offset, count) && SpiNorFlash_WaitUntilReady(self, NO_WAIT);

    while (programmed && (done < count))
    {
        uint32_t address = SpiNorFlash_AddressOf(self, blockIndex, offset + done);
        size_t length = SpiNorFlash_BytesToProgramInPage(self, address, count - done);
        programmed = SpiNorFlash_ProgramInPage(self, address, &bytes[done], length);
        done += length;
    }

    return programmed;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

/* A program that runs past the end of its page wraps to the page's start,
 * so no more than is left of the page is sent in one command. */
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- an address and a count of bytes; distinct semantics
static inline size_t SpiNorFlash_BytesToProgramInPage(
    const struct SolidSyslogSpiNorFlash* self,
    uint32_t address,
    size_t remaining
)
{
    size_t leftInPage = (size_t) (self->Geometry.PageBytes - (address % self->Geometry.PageBytes));

    return (remaining < leftInPage) ? remaining : leftInPage;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static bool SpiNorFlash_ProgramInPage(
    struct SolidSyslogSpiNorFlash* self,
    uint32_t address,
    const uint8_t* bytes,
    size_t count
)
{
    uint8_t command[COMMAND_AND_ADDRESS_LENGTH] = {0};
    SpiNorFlash_SetCommand(command, PAGE_PROGRAM, address);

    /* The part begins the program as its select rises, at the end of the
     * send. */
    return SpiNorFlash_EnableWriting(self) &&
           SpiNorFlash_BusHeld(SpiNorFlash_SendWithData(self, command, sizeof(command), bytes, count)) &&
           SpiNorFlash_WaitUntilReady(self, self->Geometry.ProgramTimeoutMilliseconds);
}

/* The part accepts a program or an erase only immediately after a write
 * enable, and its status shows whether the enable took. If it did not, the
 * part would ignore the next command without saying so. */
static bool SpiNorFlash_EnableWriting(struct SolidSyslogSpiNorFlash* self)
{
    const uint8_t command = WRITE_ENABLE;
    uint8_t status = IDLE_LINE;
    bool enabled = false;

    if (!(SpiNorFlash_Send(self, &command, sizeof(command)) && SpiNorFlash_ReadStatus(self, &status)))
    {
        SpiNorFlash_ReportIoFailure(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED);
    }
    else if (status == (uint8_t) IDLE_LINE)
    {
        SpiNorFlash_ReportIoFailure(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_ANSWER);
    }
    else if ((status & (uint8_t) STATUS_WRITE_ENABLED) == 0U)
    {
        SpiNorFlash_ReportIoFailure(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_WRITE_REFUSED);
    }
    else
    {
        enabled = true;
    }

    return enabled;
}

/* One command that is not answered. */
static bool SpiNorFlash_Send(struct SolidSyslogSpiNorFlash* self, const uint8_t* command, size_t count)
{
    struct SolidSyslogSpiNorFlashBus* bus = self->Bus;

    bus->Select(bus);
    bool sent = bus->Write(bus, command, count);
    bus->Deselect(bus);

    return sent;
}

/* One command with data after it, within one select. The data goes from
 * where the caller has it: nothing is copied. */
static bool SpiNorFlash_SendWithData(
    struct SolidSyslogSpiNorFlash* self,
    const uint8_t* command,
    size_t commandCount,
    const uint8_t* data,
    size_t dataCount
)
{
    struct SolidSyslogSpiNorFlashBus* bus = self->Bus;

    bus->Select(bus);
    bool sent = bus->Write(bus, command, commandCount) && bus->Write(bus, data, dataCount);
    bus->Deselect(bus);

    return sent;
}

static bool SpiNorFlash_Erase(struct SolidSyslogNorFlash* base, size_t blockIndex)
{
    struct SolidSyslogSpiNorFlash* self = SpiNorFlash_SelfFromBase(base);
    bool erased = false;

    if (SpiNorFlash_CheckSpan(self, blockIndex, 0U, 0U) && SpiNorFlash_WaitUntilReady(self, NO_WAIT))
    {
        uint8_t command[COMMAND_AND_ADDRESS_LENGTH] = {0};
        SpiNorFlash_SetCommand(command, self->Geometry.EraseCommand, SpiNorFlash_AddressOf(self, blockIndex, 0U));

        /* The part begins the erase as its select rises, at the end of the
         * send. */
        erased = SpiNorFlash_EnableWriting(self) &&
                 SpiNorFlash_BusHeld(SpiNorFlash_Send(self, command, sizeof(command))) &&
                 SpiNorFlash_WaitUntilReady(self, self->Geometry.EraseTimeoutMilliseconds);
    }

    return erased;
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
