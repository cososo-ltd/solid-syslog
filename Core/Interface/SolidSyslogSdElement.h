/** @file
 *  The SD authoring API for one [SD-ID PARAM="value"...] element:
 *  SolidSyslogSdElement_Begin / SolidSyslogSdElement_Param /
 *  SolidSyslogSdElement_End, which own the brackets and SD-NAME charset so the
 *  author writes only names and values. */
#ifndef SOLIDSYSLOGSDELEMENT_H
#define SOLIDSYSLOGSDELEMENT_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** The element writer handed to an SD's Format. Owns the brackets and the
     *  SD-NAME / PARAM-NAME charset (each bounded to 32 bytes), so an author
     *  writes only names and values and cannot desync the framing.
     *  Stack-transient, no pool (D.002). */
    struct SolidSyslogSdElement;
    struct SolidSyslogSdValue;

    /** Opens an SD-ELEMENT: emits "[name" for an IANA-registered name
     *  (@p enterpriseNumber 0) or "[name@enterpriseNumber" for a private one. A
     *  NULL @p name suppresses the whole element, so a conditional element needs
     *  no placeholder; the matching SolidSyslogSdElement_End is still required. */
    void SolidSyslogSdElement_Begin(struct SolidSyslogSdElement * element, const char* name, uint32_t enterpriseNumber);

    /** Opens an SD-PARAM and returns the value sink to stream its value into.
     *  Always returns a usable sink, never NULL: a NULL @p name (or a suppressed
     *  element) skips the param but still absorbs the caller's value writes. The
     *  returned pointer belongs to the element and stays valid until the next
     *  SolidSyslogSdElement_Param or SolidSyslogSdElement_End. */
    struct SolidSyslogSdValue* SolidSyslogSdElement_Param(struct SolidSyslogSdElement * element, const char* name);

    /** Closes the SD-ELEMENT: closes any open param value's quote and emits ']'
     *  (nothing if SolidSyslogSdElement_Begin suppressed the element). */
    void SolidSyslogSdElement_End(struct SolidSyslogSdElement * element);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSDELEMENT_H */
