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
 * A slot holds the most recent event of its kind: its severity as a one-byte
 * code, Source, Detail, NativeCode with its HasNativeCode flag, and a Pending
 * flag. The category is the slot's index, so it is not stored. On a 32-bit
 * target a slot is 16 bytes and the table of
 * SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT slots is 224 bytes of static RAM; the
 * kind table beside it, which also holds each kind's own severity, is 112
 * bytes of constant data, and the severities the codes index are 32 more. A
 * 64-bit host pads a slot to 24 bytes.
 *
 * The tap is the whole of what runs on a failing path after the handler, so it
 * must be safe from any task and any context, including inside a Send that is
 * failing and inside a DetailName callback. It therefore calls no role, no
 * clock, no lock and no name function. Recording is six stores with Pending
 * last, and Take clears Pending before it copies the fields. The slots are
 * volatile, so the compiler keeps the stores in that order, which is as much as
 * C99 offers: there are no atomics in the baseline, and no lock is taken
 * because the tap runs inside a failing role on whatever task met the fault. A
 * multi-core CPU may still retire the stores out of order; nothing here
 * prevents that. Two writers racing on one slot, or a Take copying while a
 * writer lands, leave a slot or a copy mixing fields of two events of the same
 * kind, with the flag set. The writers are logging tasks sharing one buffer,
 * since every other kind is raised on the Service task, so the two events come
 * from one source: both said the kind happened, the mix is a valid event of it,
 * and an event landing during a copy is reported again on the next Take rather
 * than lost. The first write of a kind is the other case: on a core that
 * reorders the stores, a Take that overtakes it can see a slot still at its
 * initial values: a NULL Source at the kind's own severity, the least severe
 * the kind is raised at. */
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

    /* Record an event in its kind's slot, overwriting whatever the slot held;
     * an event of an unreported category changes nothing. Called by
     * SolidSyslog_Error after the handler. */
    void SolidSyslogPendingEvents_Record(const struct SolidSyslogErrorEvent* event);

    /* The slot index of a reported kind, or SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT
     * for a category that is not one, which Take answers false for. */
    size_t SolidSyslogPendingEvents_IndexOf(uint16_t category);

    /* Whether the slot is pending, leaving it as it is; Take is what clears
     * it. An index past the table is not pending. */
    bool SolidSyslogPendingEvents_IsPending(size_t index);

    /* Whether any slot is pending, leaving every slot as it is. */
    bool SolidSyslogPendingEvents_AnyPending(void);

    /* Clear the slot's Pending flag, then copy its fields into @p event with
     * the slot's category. Returns whether the slot was pending; the copy is
     * made either way, so @p event is only meaningful on true. Slot order is
     * the table's own, causes first: a reader walks every index in order,
     * so it takes a cause ahead of what it caused, and reads the category
     * from the event. An index past the table leaves @p event untouched and
     * returns false. */
    bool SolidSyslogPendingEvents_Take(size_t index, struct SolidSyslogErrorEvent* event);

    /* Clear every slot, leaving nothing pending and each slot at its kind's
     * own severity, as at start. Called by SolidSyslog_Destroy, so a logger
     * created afterwards reports only what happens after it. */
    void SolidSyslogPendingEvents_Clear(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPENDINGEVENTS_H */
