#ifndef SOLIDSYSLOGPOSIXHOSTNAME_H
#define SOLIDSYSLOGPOSIXHOSTNAME_H

#include "ExternC.h"

struct SolidSyslogHeaderField;

EXTERN_C_BEGIN

    void SolidSyslogPosixHostname_Get(struct SolidSyslogHeaderField * field, void* context);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXHOSTNAME_H */
