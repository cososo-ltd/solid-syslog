#ifndef ERRORHANDLERFAKE_H
#define ERRORHANDLERFAKE_H

#include <stdint.h>

#include "SolidSyslogExternC.h"
#include "SolidSyslogPrival.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    void ErrorHandlerFake_Install(void* context);
    int ErrorHandlerFake_HandleCallCount(void);
    enum SolidSyslogSeverity ErrorHandlerFake_LastSeverity(void);
    const struct SolidSyslogErrorSource* ErrorHandlerFake_LastSource(void);
    uint16_t ErrorHandlerFake_LastCategory(void);
    int32_t ErrorHandlerFake_LastDetail(void);
    const void* ErrorHandlerFake_LastContext(void);

    /* The index-th event since Install, counting from 0, for a call under test
     * that raises more than one. An index past what was kept answers the
     * values Install reset to. */
    enum SolidSyslogSeverity ErrorHandlerFake_SeverityAt(int index);
    const struct SolidSyslogErrorSource* ErrorHandlerFake_SourceAt(int index);
    uint16_t ErrorHandlerFake_CategoryAt(int index);
    int32_t ErrorHandlerFake_DetailAt(int index);

SOLIDSYSLOG_EXTERN_C_END

#endif /* ERRORHANDLERFAKE_H */
