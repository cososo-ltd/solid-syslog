/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The header-field callback typedef the integrator supplies to
 *  SolidSyslogConfig for HOSTNAME / APP-NAME / PROCID. */
#ifndef SOLIDSYSLOGHEADERFIELDFUNCTION_H
#define SOLIDSYSLOGHEADERFIELDFUNCTION_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogHeaderField;

    /** Appends an RFC 5424 header-field value (HOSTNAME / APP-NAME / PROCID) into
     *  the field sink it is handed. The sink owns the charset and framing, so a
     *  callback cannot break the header. @p context is passed through unchanged
     *  from the config the callback was registered in. */
    typedef void (*SolidSyslogHeaderFieldFunction)(struct SolidSyslogHeaderField* field, void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGHEADERFIELDFUNCTION_H */
