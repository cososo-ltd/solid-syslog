#ifndef SOLIDSYSLOGSTRINGFUNCTION_H
#define SOLIDSYSLOGSTRINGFUNCTION_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogFormatter;

    typedef void (*SolidSyslogStringFunction)(struct SolidSyslogFormatter* formatter);

EXTERN_C_END

#endif /* SOLIDSYSLOGSTRINGFUNCTION_H */
