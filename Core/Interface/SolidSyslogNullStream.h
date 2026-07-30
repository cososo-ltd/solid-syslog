/** @file
 *  The no-op Stream Null object: Send returns true (drops the bytes on the floor so the
 *  Store does not fill with undeliverables), Read returns 0 (would-block, so the caller
 *  does not tear the connection down), Open and Close are no-ops. */
#ifndef SOLIDSYSLOGNULLSTREAM_H
#define SOLIDSYSLOGNULLSTREAM_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Open and Close are no-ops. Send returns true, reporting the bytes as delivered
     *  so the Store does not retain undeliverables. Read returns 0 (would-block, nothing
     *  available) rather than a negative value, so the caller does not treat the
     *  connection as broken and tear it down. */
    struct SolidSyslogStream* SolidSyslogNullStream_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLSTREAM_H */
