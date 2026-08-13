/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The Buffer vtable (Write / Read) - the contract an implementor fills in (the
 *  Buffer extension point). */
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
     *  so, which leaves the overflow policy and whether a drop is reported to
     *  the implementation. */
    struct SolidSyslogBuffer
    {
        void (*Write)(struct SolidSyslogBuffer* base, const void* data, size_t size);
        bool (*Read)(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGBUFFERDEFINITION_H */
