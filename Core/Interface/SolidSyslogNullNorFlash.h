/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The no-op NorFlash Null object: every method reports a flash that is not
 *  there - Read, Program and Erase return false, and GetBlockSize and
 *  GetBlockCount return 0. */
#ifndef SOLIDSYSLOGNULLNORFLASH_H
#define SOLIDSYSLOGNULLNORFLASH_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogNorFlash;

    /** Every method reports a flash that is not there: Read, Program and Erase
     *  return false, and GetBlockSize and GetBlockCount return 0. */
    struct SolidSyslogNorFlash* SolidSyslogNullNorFlash_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLNORFLASH_H */
