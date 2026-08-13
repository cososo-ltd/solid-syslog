/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The structured-data role: emit one or more RFC 5424 SD-ELEMENTs into a
 *  message (Format). This call dispatches to the injected SD's vtable, so the
 *  content - and whether it is stateful - is that SD's. */
#ifndef SOLIDSYSLOGSTRUCTUREDDATA_H
#define SOLIDSYSLOGSTRUCTUREDDATA_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogSdElement;
    struct SolidSyslogStructuredData;

    /** Dispatch to @p sd's Format, writing one or more SD-ELEMENTs into
     *  @p element. Called by the library while it builds a message; @p element
     *  is an SD-ELEMENT writer that owns the framing, so an SD cannot break it.
     *  A stateful SD reads its data through @p sd (the vtable is its first
     *  member), so it must stay valid for the log call and must not be shared
     *  mutably across threads logging concurrently. */
    void SolidSyslogStructuredData_Format(struct SolidSyslogStructuredData * sd, struct SolidSyslogSdElement * element);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSTRUCTUREDDATA_H */
