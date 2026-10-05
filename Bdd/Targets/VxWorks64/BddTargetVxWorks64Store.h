#ifndef BDDTARGETVXWORKS64STORE_H
#define BDDTARGETVXWORKS64STORE_H

#include <stdbool.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Readies the disk the file store lives on: dosFs, formatted on first use. */
    bool BddTargetVxWorks64Store_Mount(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETVXWORKS64STORE_H */
