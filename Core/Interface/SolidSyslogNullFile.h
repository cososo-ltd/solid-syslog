/** @file
 *  The no-op File Null object: Open, IsOpen, Read and Exists return false (consumers take
 *  their error path), Write and Delete return true (success reported vacuously), SeekTo,
 *  Truncate and Close are no-ops, Size returns 0. */
#ifndef SOLIDSYSLOGNULLFILE_H
#define SOLIDSYSLOGNULLFILE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Open, IsOpen, Read and Exists return false, so consumers see a consistently
     *  non-functional file and take their error path. Write and Delete return true,
     *  reporting success vacuously so callers that treat those as no-ops are not tripped.
     *  SeekTo, Truncate and Close are no-ops; Size returns 0. */
    struct SolidSyslogFile* SolidSyslogNullFile_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLFILE_H */
