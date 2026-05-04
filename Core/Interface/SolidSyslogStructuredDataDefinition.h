#ifndef SOLIDSYSLOGSTRUCTUREDDATADEFINITION_H
#define SOLIDSYSLOGSTRUCTUREDDATADEFINITION_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogFormatter;

    struct SolidSyslogStructuredData
    {
        void (*Format)(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGSTRUCTUREDDATADEFINITION_H */
