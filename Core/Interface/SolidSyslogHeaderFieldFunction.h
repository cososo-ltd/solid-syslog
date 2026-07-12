#ifndef SOLIDSYSLOGHEADERFIELDFUNCTION_H
#define SOLIDSYSLOGHEADERFIELDFUNCTION_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogHeaderField;

    /** Appends an RFC 5424 header-field value (HOSTNAME / APP-NAME / PROCID) into
     *  the field sink it is handed. The sink owns the charset and framing, so a
     *  callback cannot break the header. @p context is passed through unchanged
     *  from the config the callback was registered in. */
    typedef void (*SolidSyslogHeaderFieldFunction)(struct SolidSyslogHeaderField* field, void* context);

EXTERN_C_END

#endif /* SOLIDSYSLOGHEADERFIELDFUNCTION_H */
