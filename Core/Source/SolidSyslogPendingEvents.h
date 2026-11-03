/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* The pending-event table: the library's record of its own reportable
 * faults, written by a tap in SolidSyslog_Error after the handler returns and
 * read by whatever turns a pending kind into a syslog record.
 *
 * One static slot per reported kind, keyed by the portable category. The
 * kinds are the categories a collector needs to hear about and that today's
 * code already raises: the losses and their causes, the delivery state, and
 * a rejected peer. A category outside the set is not recorded. The library
 * picks the set; there is no integrator selection.
 *
 * A slot holds the most recent event of its kind: Severity, Source, Detail,
 * NativeCode with its HasNativeCode flag, and a Pending flag. The category is
 * the slot's index, so it is not stored. On a 32-bit target a slot is 20 bytes
 * and the table of SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT slots is 280 bytes of
 * static RAM; the kind table beside it is 28 bytes of constant data. A 64-bit
 * host pads a slot to 32 bytes.
 *
 * The tap is the whole of what runs on a failing path after the handler, so
 * it must be safe from any task and any context, including inside a Send
 * that is failing and inside a DetailName callback. It therefore calls
 * nothing: no role, no clock, no lock, no name function. Recording is a
 * handful of plain word-sized stores with Pending written last, and Take
 * clears Pending before it copies the fields. The ordering is stated here
 * rather than enforced by volatile or an atomic: volatile on one member
 * orders nothing else against it, and the C99 baseline has no atomics. What
 * the stores do guarantee is that every field of a slot is always a valid
 * value for that kind, so two writers racing on one slot, or a Take copying
 * while a writer lands, leave at worst a mix of two events of the same kind
 * with a set flag. Both said the kind happened, and an event landing during
 * a copy is reported again on the next Take rather than lost. */
#ifndef SOLIDSYSLOGPENDINGEVENTS_H
#define SOLIDSYSLOGPENDINGEVENTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogExternC.h"

/* The number of reported kinds, and so of slots. */
#define SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT 14U

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Clear every slot's Pending flag. */
    void SolidSyslogPendingEvents_Reset(void);

    /* The slot index of a category, or SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT
     * for a category that is not a reported kind. */
    size_t SolidSyslogPendingEvents_IndexOf(uint16_t category);

    /* Record an event in its kind's slot, overwriting whatever the slot held;
     * an event of an unreported category changes nothing. Called by
     * SolidSyslog_Error after the handler. */
    void SolidSyslogPendingEvents_Record(const struct SolidSyslogErrorEvent* event);

    /* Clear the slot's Pending flag, then copy its fields into @p event with
     * the slot's category. Returns whether the slot was pending; the copy is
     * made either way, so @p event is only meaningful on true. An index past
     * the table leaves @p event untouched and returns false. */
    bool SolidSyslogPendingEvents_Take(size_t index, struct SolidSyslogErrorEvent* event);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPENDINGEVENTS_H */
