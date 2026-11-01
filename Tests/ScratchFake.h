#ifndef SCRATCHFAKE_H
#define SCRATCHFAKE_H

#include "SolidSyslogExternC.h"

#include <stddef.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogScratch;

    /* Storage for a max-size message until ScratchFake_SetMessageBytes says
       otherwise; counts every WithScratch. */
    struct SolidSyslogScratch* ScratchFake_Create(void);
    void ScratchFake_Destroy(void);
    void ScratchFake_SetMessageBytes(size_t messageBytes);
    int ScratchFake_CallCount(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SCRATCHFAKE_H */
