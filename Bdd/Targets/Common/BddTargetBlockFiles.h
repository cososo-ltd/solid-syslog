#ifndef BDDTARGETBLOCKFILES_H
#define BDDTARGETBLOCKFILES_H

#include <stdio.h>

#include "SolidSyslogExternC.h"

struct SolidSyslogBlockDevice;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The reply to the harness's `blocks` command: one line, `BLOCKS:` then
       the index, in at least two digits, of each block the device says is
       present. Every block the device has is asked for; the retention limit
       does not bound the index. */
    void BddTargetBlockFiles_Print(FILE * out, struct SolidSyslogBlockDevice * device);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETBLOCKFILES_H */
