/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The no-op BlockDevice Null object: every method reports a device that does not exist -
 *  Acquire, Dispose, Exists, Read, Append and WriteAt return false, Size fails with a size
 *  of 0 - a failed device, never an empty block - and GetBlockSize returns 0. */
#ifndef SOLIDSYSLOGNULLBLOCKDEVICE_H
#define SOLIDSYSLOGNULLBLOCKDEVICE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogBlockDevice;

    /** Every method reports a device that does not exist: Acquire, Dispose, Exists,
     *  Read, Append and WriteAt all return false, Size fails with a size of 0 - a failed
     *  device, never an empty block - and GetBlockSize returns 0. */
    struct SolidSyslogBlockDevice* SolidSyslogNullBlockDevice_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLBLOCKDEVICE_H */
