/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "vxWorks.h"

#include <sys/stat.h>

#include "BddTargetVxWorks64Store.h"

#include <stddef.h>
#include <stdbool.h>

#include "dosFsLib.h"
#include "hrfsLib.h"

/* The primary IDE disk, as INCLUDE_ATA names it. The formatters take the name
 * as a char* they only read, so it lives in an array. */
static char volume[] = "/ata0a";

/* dosFsVolFormat's options: none, for the default FAT format. hrfsFormat
 * given zero for the disk size, block size and inode count uses the whole disk
 * and chooses the other two itself. */
enum
{
    DOSFS_DEFAULT_FORMAT = 0,
    HRFS_FORMATTER_CHOOSES = 0
};

static bool BddTargetVxWorks64Store_IsFormatted(void);
static bool BddTargetVxWorks64Store_Holds(enum BddTargetVxWorks64FileSystem fileSystem);
static bool BddTargetVxWorks64Store_Format(enum BddTargetVxWorks64FileSystem fileSystem);

/* A blank disk is formatted with the file system chosen, and is then that file
 * system at once; one already formatted keeps what it holds, which is what
 * lets the store outlive a power cycle - but only if it is the one chosen. */
enum BddTargetVxWorks64StoreMountResult BddTargetVxWorks64Store_Mount(enum BddTargetVxWorks64FileSystem fileSystem)
{
    enum BddTargetVxWorks64StoreMountResult result = BDD_TARGET_VXWORKS64_STORE_MOUNTED;
    if (BddTargetVxWorks64Store_IsFormatted())
    {
        if (!BddTargetVxWorks64Store_Holds(fileSystem))
        {
            result = BDD_TARGET_VXWORKS64_STORE_OTHER_FILE_SYSTEM;
        }
    }
    else if (!BddTargetVxWorks64Store_Format(fileSystem))
    {
        result = BDD_TARGET_VXWORKS64_STORE_FORMAT_FAILED;
    }
    else
    {
        /* Formatted, and so mounted. */
    }
    return result;
}

/* The file system monitor puts rawFs on a disk it cannot recognise, and rawFs
 * answers no file status; formatted, the volume's root is a directory. */
static bool BddTargetVxWorks64Store_IsFormatted(void)
{
    struct stat status = {0};
    bool answers = stat("/ata0a/", &status) == OK;
    return answers && ((status.st_mode & S_IFMT) == S_IFDIR);
}

/* dosFs answers for the volumes it owns. HRFS has no lookup by path, so a
 * formatted volume dosFs does not own is HRFS - true only because the image
 * carries no third file system that formats. */
static bool BddTargetVxWorks64Store_Holds(enum BddTargetVxWorks64FileSystem fileSystem)
{
    bool isDosFs = dosFsVolDescGet(volume, NULL) != NULL;
    return isDosFs == (fileSystem == BDD_TARGET_VXWORKS64_FILE_SYSTEM_DOSFS);
}

static bool BddTargetVxWorks64Store_Format(enum BddTargetVxWorks64FileSystem fileSystem)
{
    bool formatted = false;
    if (fileSystem == BDD_TARGET_VXWORKS64_FILE_SYSTEM_HRFS)
    {
        formatted = hrfsFormat(volume, HRFS_FORMATTER_CHOOSES, HRFS_FORMATTER_CHOOSES, HRFS_FORMATTER_CHOOSES) == OK;
    }
    else
    {
        formatted = dosFsVolFormat(volume, DOSFS_DEFAULT_FORMAT, NULL) == OK;
    }
    return formatted;
}
