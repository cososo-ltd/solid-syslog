#ifndef BDDTARGETERRORTEXT_H
#define BDDTARGETERRORTEXT_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorEvent;

    const char* BddTargetErrorText_Category(uint16_t category);
    const char* BddTargetErrorText_Detail(const struct SolidSyslogErrorEvent* event);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETERRORTEXT_H */
