/** @file
 *  The SD-value callback typedef the integrator supplies for dynamic PARAM
 *  values (e.g. MetaSd language, OriginSd fields). */
#ifndef SOLIDSYSLOGSDVALUEFUNCTION_H
#define SOLIDSYSLOGSDVALUEFUNCTION_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogSdValue;

    /** Streams an SD-PARAM value into the @p value sink it is handed. The sink
     *  applies the escaping, so a callback cannot break SD framing regardless of
     *  the bytes it writes. @p context is passed through unchanged from the
     *  config the callback was registered in. */
    typedef void (*SolidSyslogSdValueFunction)(struct SolidSyslogSdValue* value, void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSDVALUEFUNCTION_H */
