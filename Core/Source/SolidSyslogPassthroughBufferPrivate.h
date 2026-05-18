#ifndef SOLIDSYSLOGPASSTHROUGHBUFFERPRIVATE_H
#define SOLIDSYSLOGPASSTHROUGHBUFFERPRIVATE_H

#include "SolidSyslogBufferDefinition.h"

struct SolidSyslogSender;

struct SolidSyslogPassthroughBuffer
{
    struct SolidSyslogBuffer Base;
    struct SolidSyslogSender* Sender;
};

void PassthroughBuffer_Initialise(struct SolidSyslogBuffer* base, struct SolidSyslogSender* sender);
void PassthroughBuffer_Cleanup(struct SolidSyslogBuffer* base);

#endif /* SOLIDSYSLOGPASSTHROUGHBUFFERPRIVATE_H */
