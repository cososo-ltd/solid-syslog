/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The no-op Sender Null object: Send returns true (drops the record on the floor so the
 *  Store does not fill with undeliverables), Disconnect is a no-op. */
#ifndef SOLIDSYSLOGNULLSENDER_H
#define SOLIDSYSLOGNULLSENDER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Send returns true, reporting the record as delivered so the Store drops it
     *  rather than accumulating undeliverables. Disconnect is a no-op. */
    struct SolidSyslogSender* SolidSyslogNullSender_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLSENDER_H */
