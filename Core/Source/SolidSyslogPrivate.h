#ifndef SOLIDSYSLOGPRIVATE_H
#define SOLIDSYSLOGPRIVATE_H

#include "SolidSyslogError.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogMessageFormatter.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTimestamp.h"

struct SolidSyslogBuffer;
struct SolidSyslogConfig;
struct SolidSyslogFormatter;
struct SolidSyslogSender;
struct SolidSyslogStore;

struct SolidSyslog
{
    struct SolidSyslogBuffer* Buffer;
    struct SolidSyslogSender* Sender;
    struct SolidSyslogStore* Store;
    struct SolidSyslogMessageFormatterContext Format;
};

void SolidSyslog_Initialise(struct SolidSyslog* self, const struct SolidSyslogConfig* config);
void SolidSyslog_Cleanup(struct SolidSyslog* self);

/* No-op function-pointer defaults shared between SolidSyslog.c and
 * SolidSyslogStatic.c (the latter wires them into the exhaustion-fallback
 * NullInstance). No public Null equivalent exists for the function-pointer
 * typedefs, so they stay TU-internal across this class. */
void SolidSyslog_NullClock(struct SolidSyslogTimestamp* ts);
void SolidSyslog_NullStringFunction(struct SolidSyslogFormatter* formatter);

static inline void SolidSyslog_Report(enum SolidSyslogSeverity severity, uint16_t category, enum SolidSyslogErrors code)
{
    SolidSyslog_Error(severity, &SolidSyslogErrorSource, category, code);
}

#endif /* SOLIDSYSLOGPRIVATE_H */
