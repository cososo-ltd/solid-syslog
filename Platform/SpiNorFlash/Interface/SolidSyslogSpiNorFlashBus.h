/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The SPI bus a SolidSyslogSpiNorFlash reaches its part over (Select /
 *  Deselect / Write / Read / Milliseconds) - the table of functions an
 *  integrator fills in for the board the part is on. */
#ifndef SOLIDSYSLOGSPINORFLASHBUS_H
#define SOLIDSYSLOGSPINORFLASHBUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** The bus under a SolidSyslogSpiNorFlash: an integrator fills this table and
     *  embeds it as the first member of its own struct, so @p base downcasts back
     *  to that struct. Every member is filled; there is no optional one.
     *
     *  The bus is ready before the flash is created: its pins, its mode and its
     *  rate are the integrator's, and so is anything another device on the same
     *  wires needs before it will stay off them. SPI mode 0, most significant
     *  bit first.
     *
     *  The flash frames each command with Select and Deselect, and sends and
     *  receives only between them. The part acts on a command as its select
     *  rises, so Deselect returns with the select high.
     *
     *  Every call returns in bounded time. A bus that waits on a transfer gives
     *  up after a time of its own and answers false; the flash then fails the
     *  operation it was carrying out. The flash bounds the time it waits for
     *  the part, by Milliseconds, and cannot bound a bus call that never
     *  returns.
     *
     *  Access is single-threaded, as the flash above is. A bus shared with
     *  another device takes whatever lock that needs, held from Select to
     *  Deselect; the library holds none. */
    struct SolidSyslogSpiNorFlashBus
    {
        /** Drive the part's select low. */
        void (*Select)(struct SolidSyslogSpiNorFlashBus* base);
        /** Drive the part's select high. */
        void (*Deselect)(struct SolidSyslogSpiNorFlashBus* base);
        /** Clock @p count bytes out of @p buffer, discarding what is clocked in.
         *  @retval false the bus did not send them all. */
        bool (*Write)(struct SolidSyslogSpiNorFlashBus* base, const void* buffer, size_t count);
        /** Clock @p count bytes into @p buffer. What is clocked out meanwhile is
         *  the bus's choice: the part ignores it. A bus with no part on it reads
         *  0xFF, and that is a true.
         *  @retval false the bus did not clock them all in; @p buffer is
         *          undefined. */
        bool (*Read)(struct SolidSyslogSpiNorFlashBus* base, void* buffer, size_t count);
        /** A count of milliseconds that wraps at 2^32, for timing the part. Only
         *  differences between two readings are used. */
        uint32_t (*Milliseconds)(struct SolidSyslogSpiNorFlashBus* base);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSPINORFLASHBUS_H */
