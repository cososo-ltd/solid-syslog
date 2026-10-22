#ifndef BDDTARGETBLOCKFILES_H
#define BDDTARGETBLOCKFILES_H

#include <stdio.h>

#include "SolidSyslogExternC.h"

struct SolidSyslogFile;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The reply to the harness's `blocks` command: one line, `BLOCKS:` then
       the two-digit index of each block file present under pathPrefix, asked
       of the pack's own File. Every index the block sequence can use is
       asked for; the retention limit does not bound the index. */
    void BddTargetBlockFiles_Print(FILE * out, struct SolidSyslogFile * file, const char* pathPrefix);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETBLOCKFILES_H */
