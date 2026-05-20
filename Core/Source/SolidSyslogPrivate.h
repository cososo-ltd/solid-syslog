#ifndef SOLIDSYSLOGPRIVATE_H
#define SOLIDSYSLOGPRIVATE_H

#include <stddef.h>

#include "SolidSyslogStringFunction.h"
#include "SolidSyslogTimestamp.h"

struct SolidSyslogBuffer;
struct SolidSyslogConfig;
struct SolidSyslogFormatter;
struct SolidSyslogSender;
struct SolidSyslogStore;
struct SolidSyslogStructuredData;

struct SolidSyslog
{
    struct SolidSyslogBuffer* Buffer;
    struct SolidSyslogSender* Sender;
    SolidSyslogClockFunction Clock;
    SolidSyslogStringFunction GetHostname;
    SolidSyslogStringFunction GetAppName;
    SolidSyslogStringFunction GetProcessId;
    struct SolidSyslogStore* Store;
    struct SolidSyslogStructuredData** Sd;
    size_t SdCount;
};

void SolidSyslog_Initialise(struct SolidSyslog* self, const struct SolidSyslogConfig* config);
void SolidSyslog_Cleanup(struct SolidSyslog* self);

/* No-op function-pointer defaults shared between SolidSyslog.c and
 * SolidSyslogStatic.c (the latter wires them into the exhaustion-fallback
 * NullInstance). No public Null equivalent exists for the function-pointer
 * typedefs, so they stay TU-internal across this class. */
void SolidSyslog_NullClock(struct SolidSyslogTimestamp* ts);
void SolidSyslog_NullStringFunction(struct SolidSyslogFormatter* formatter);

#endif /* SOLIDSYSLOGPRIVATE_H */
