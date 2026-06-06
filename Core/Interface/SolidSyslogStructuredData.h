#ifndef SOLIDSYSLOGSTRUCTUREDDATA_H
#define SOLIDSYSLOGSTRUCTUREDDATA_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogSdElement;
    struct SolidSyslogStructuredData;

    void SolidSyslogStructuredData_Format(struct SolidSyslogStructuredData * sd, struct SolidSyslogSdElement * element);

EXTERN_C_END

#endif /* SOLIDSYSLOGSTRUCTUREDDATA_H */
