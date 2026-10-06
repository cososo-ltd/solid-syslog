/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "vxWorks.h"

#include <sys/stat.h>

#include "BddTargetVxWorks64Store.h"

#include <stddef.h>
#include <stdbool.h>

#include "dosFsLib.h"

/* The primary IDE disk, as INCLUDE_ATA names it. dosFsVolFormat takes the name
 * as a char* it only reads, so it lives in an array. */
static char volume[] = "/ata0a";

/* dosFsVolFormat's options: none, for the default FAT format. */
enum
{
    DOSFS_DEFAULT_FORMAT = 0
};

static bool BddTargetVxWorks64Store_IsDosFs(void);

/* A blank disk is formatted, and is then dosFs at once; one already formatted
 * keeps what it holds, which is what lets the store outlive a power cycle. */
bool BddTargetVxWorks64Store_Mount(void)
{
    bool ready = BddTargetVxWorks64Store_IsDosFs();
    if (!ready)
    {
        ready = dosFsVolFormat(volume, DOSFS_DEFAULT_FORMAT, NULL) == OK;
    }
    return ready;
}

/* The file system monitor puts rawFs on a disk it cannot recognise, and rawFs
 * answers no file status; formatted, the volume's root is a directory. */
static bool BddTargetVxWorks64Store_IsDosFs(void)
{
    struct stat status = {0};
    bool isDosFs = stat("/ata0a/", &status) == OK;
    return isDosFs && ((status.st_mode & S_IFMT) == S_IFDIR);
}
