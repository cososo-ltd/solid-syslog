#ifndef ERRORHANDLERFAKE_H
#define ERRORHANDLERFAKE_H

#include <stdint.h>

#include "ExternC.h"
#include "SolidSyslogPrival.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    void ErrorHandlerFake_Install(void* context);
    int ErrorHandlerFake_HandleCallCount(void);
    enum SolidSyslogSeverity ErrorHandlerFake_LastSeverity(void);
    const struct SolidSyslogErrorSource* ErrorHandlerFake_LastSource(void);
    uint16_t ErrorHandlerFake_LastCategory(void);
    int32_t ErrorHandlerFake_LastDetail(void);
    const void* ErrorHandlerFake_LastContext(void);

EXTERN_C_END

#endif /* ERRORHANDLERFAKE_H */
