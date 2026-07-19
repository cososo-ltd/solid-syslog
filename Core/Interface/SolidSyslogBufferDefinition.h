/** @file
 *  The Buffer vtable (Write / Read) — the contract an implementor fills in (the
 *  Buffer extension point). */
#ifndef SOLIDSYSLOGBUFFERDEFINITION_H
#define SOLIDSYSLOGBUFFERDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    /** Vtable an implementation embeds as its first member (base) and downcasts
     *  from. Write and Read carry the SolidSyslogBuffer_Write / _Read contract;
     *  an implementation that queues must make the two sides mutually safe
     *  itself, since Service (Read) and Log (Write) can run on different tasks.
     *  Read returns false for a head record too large for the caller's buffer and
     *  reports it via SolidSyslog_Error (buffer-backend-failed) — that state
     *  cannot arise under correct configuration, so it must not be silent. */
    struct SolidSyslogBuffer
    {
        void (*Write)(struct SolidSyslogBuffer* base, const void* data, size_t size);
        bool (*Read)(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGBUFFERDEFINITION_H */
