#ifndef SOLIDSYSLOGSDELEMENT_H
#define SOLIDSYSLOGSDELEMENT_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    /* The element writer handed to an SD's Format. Owns SD framing and the
     * SD-NAME / PARAM-NAME charset so an author cannot desync the framing.
     * Stack-transient, no pool (D.002). */
    struct SolidSyslogSdElement;
    struct SolidSyslogSdValue;

    /* Opens an SD-ELEMENT: emits "[name" for an IANA name (enterpriseNumber 0)
     * or "[name@enterpriseNumber" otherwise. */
    void SolidSyslogSdElement_Begin(struct SolidSyslogSdElement * element, const char* name, uint32_t enterpriseNumber);

    /* Opens an SD-PARAM: emits ' name="' and returns the value sink the caller
     * streams the param value into. The returned pointer is owned by the
     * element and stays valid until the next _Param or _End. */
    struct SolidSyslogSdValue* SolidSyslogSdElement_Param(struct SolidSyslogSdElement * element, const char* name);

EXTERN_C_END

#endif /* SOLIDSYSLOGSDELEMENT_H */
