#ifndef BDDTARGETBLOCKFILES_H
#define BDDTARGETBLOCKFILES_H

#include <stddef.h>
#include <stdio.h>

#include "SolidSyslogExternC.h"

struct SolidSyslogFile;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The reply to the harness's `blocks` command: one line, `BLOCKS:` then
       the two-digit index of each block file present under pathPrefix, asked
       of the pack's own File. */
    void BddTargetBlockFiles_Print(
        FILE* out,
        struct SolidSyslogFile* file,
        const char* pathPrefix,
        size_t maxBlocks
    );

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETBLOCKFILES_H */
