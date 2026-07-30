/** @file
 *  The no-op Buffer Null object: Write swallows the record, Read returns false (nothing to
 *  deliver) so the Service algorithm sees an empty buffer and stops draining. */
#ifndef SOLIDSYSLOGNULLBUFFER_H
#define SOLIDSYSLOGNULLBUFFER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogBuffer;

    /** Write swallows the record. Read returns false (nothing to deliver), so the
     *  Service algorithm sees an empty buffer and stops draining. */
    struct SolidSyslogBuffer* SolidSyslogNullBuffer_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLBUFFER_H */
