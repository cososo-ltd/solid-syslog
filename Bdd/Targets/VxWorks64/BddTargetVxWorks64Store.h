/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef BDDTARGETVXWORKS64STORE_H
#define BDDTARGETVXWORKS64STORE_H

#include <stdbool.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The file systems the image carries, either of which the store can live on. */
    enum BddTargetVxWorks64FileSystem
    {
        BDD_TARGET_VXWORKS64_FILE_SYSTEM_DOSFS,
        BDD_TARGET_VXWORKS64_FILE_SYSTEM_HRFS
    };

    enum BddTargetVxWorks64StoreMountResult
    {
        BDD_TARGET_VXWORKS64_STORE_MOUNTED,
        BDD_TARGET_VXWORKS64_STORE_FORMAT_FAILED,
        /* Formatted already, but not with the file system asked for. */
        BDD_TARGET_VXWORKS64_STORE_OTHER_FILE_SYSTEM
    };

    /* Readies the disk the file store lives on, formatted on first use. */
    enum BddTargetVxWorks64StoreMountResult BddTargetVxWorks64Store_Mount(enum BddTargetVxWorks64FileSystem fileSystem);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETVXWORKS64STORE_H */
