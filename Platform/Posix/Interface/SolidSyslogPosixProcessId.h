#ifndef SOLIDSYSLOGPOSIXPROCESSID_H
#define SOLIDSYSLOGPOSIXPROCESSID_H

#include "ExternC.h"

struct SolidSyslogHeaderField;

EXTERN_C_BEGIN

    void SolidSyslogPosixProcessId_Get(struct SolidSyslogHeaderField * field, void* context);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXPROCESSID_H */
