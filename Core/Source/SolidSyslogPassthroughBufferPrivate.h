#ifndef SOLIDSYSLOGPASSTHROUGHBUFFERPRIVATE_H
#define SOLIDSYSLOGPASSTHROUGHBUFFERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPassthroughBufferErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogSender;

struct SolidSyslogPassthroughBuffer
{
    struct SolidSyslogBuffer Base;
    struct SolidSyslogSender* Sender;
};

void PassthroughBuffer_Initialise(struct SolidSyslogBuffer* base, struct SolidSyslogSender* sender);
void PassthroughBuffer_Cleanup(struct SolidSyslogBuffer* base);

static inline void PassthroughBuffer_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPassthroughBufferErrors code
)
{
    SolidSyslog_Error(severity, &PassthroughBufferErrorSource, category, code);
}

#endif /* SOLIDSYSLOGPASSTHROUGHBUFFERPRIVATE_H */
