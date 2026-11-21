#ifndef BDD_TARGET_COUNTING_NOR_FLASH_H
#define BDD_TARGET_COUNTING_NOR_FLASH_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stddef.h>

struct SolidSyslogNorFlash;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* A NorFlash that passes every call to another and counts the erases,
     * programs and reads among them, so that what a store costs its flash
     * can be measured on the part. There is one. */

    /* Puts @p beneath under the counting flash, starts the counts again, and
     * answers the counting flash, which is the same one every time. NULL
     * takes away what was beneath: the counting flash must not then be used
     * until another is given. */
    struct SolidSyslogNorFlash* BddTargetCountingNorFlash_Over(struct SolidSyslogNorFlash * beneath);

    /* Writes the counts into @p text as one line with no newline. Answers
     * false, and writes nothing, while there is no flash beneath. */
    bool BddTargetCountingNorFlash_Describe(char* text, size_t size);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_COUNTING_NOR_FLASH_H */
