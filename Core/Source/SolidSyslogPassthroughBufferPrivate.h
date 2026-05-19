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

/* Class-private GoF null Buffer. Defined in SolidSyslogPassthroughBufferStatic.c;
 * used both as the pool-exhausted fallback handle returned by _Create, and as the
 * vtable swapped into a slot's Base by _Cleanup so use-after-destroy is a safe
 * no-op. No shared SolidSyslogNullBuffer exists for E11 — PassthroughBuffer is
 * itself the closest thing to a "just forward" Buffer. */
extern struct SolidSyslogBuffer PassthroughBuffer_Fallback;

#endif /* SOLIDSYSLOGPASSTHROUGHBUFFERPRIVATE_H */
