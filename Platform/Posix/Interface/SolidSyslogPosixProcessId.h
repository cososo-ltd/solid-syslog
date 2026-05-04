#ifndef SOLIDSYSLOGPOSIXPROCESSID_H
#define SOLIDSYSLOGPOSIXPROCESSID_H

#include "ExternC.h"

struct SolidSyslogFormatter;

EXTERN_C_BEGIN

    void SolidSyslogPosixProcessId_Get(struct SolidSyslogFormatter * formatter);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXPROCESSID_H */
