/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The Buffer vtable (Write / Read / IsFull) - the contract an implementor fills
 *  in (the Buffer extension point). */
#ifndef SOLIDSYSLOGBUFFERDEFINITION_H
#define SOLIDSYSLOGBUFFERDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Vtable an implementation embeds as its first member (base) and downcasts
     *  from. Write and Read carry the SolidSyslogBuffer_Write /
     *  SolidSyslogBuffer_Read contract; an implementation that queues must make
     *  the two sides mutually safe itself, since Service (Read) and Log (Write)
     *  can run on different tasks.
     *  Read returns false for a head record too large for the caller's buffer and
     *  reports it via SolidSyslog_Error (buffer-backend-failed) - that state
     *  cannot arise under correct configuration, so it must not be silent.
     *
     *  Records come back in the order they went in, and one Read delivers
     *  exactly the bytes one Write was given: nothing downstream re-orders or
     *  re-frames them. Write cannot refuse a record, since it has no way to say
     *  so: an implementation that queues takes the discard policy at its Create
     *  and applies it when it is full. A record it loses for lack of room,
     *  refused or evicted, is reported as SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST
     *  describes; one dropped for another reason is reported under its own
     *  category. Either way the report is raised outside any lock Write holds,
     *  and its run is marked reported first, so a report that leads back into
     *  Write cannot repeat without bound.
     *
     *  IsFull carries the SolidSyslogBuffer_IsFull contract: true once a Write
     *  has found no room for a record - evicting to make some, or refusing it -
     *  and nothing has been taken out since. A buffer that never queues answers
     *  false. */
    struct SolidSyslogBuffer
    {
        void (*Write)(struct SolidSyslogBuffer* base, const void* data, size_t size);
        bool (*Read)(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
        bool (*IsFull)(struct SolidSyslogBuffer* base);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGBUFFERDEFINITION_H */
