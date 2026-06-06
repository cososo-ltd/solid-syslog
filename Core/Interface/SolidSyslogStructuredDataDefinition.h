#ifndef SOLIDSYSLOGSTRUCTUREDDATADEFINITION_H
#define SOLIDSYSLOGSTRUCTUREDDATADEFINITION_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogSdElement;

    struct SolidSyslogStructuredData
    {
        void (*Format)(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGSTRUCTUREDDATADEFINITION_H */
