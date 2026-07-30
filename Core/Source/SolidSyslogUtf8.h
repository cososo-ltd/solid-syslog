#ifndef SOLIDSYSLOGUTF8_H
#define SOLIDSYSLOGUTF8_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Byte-level UTF-8 lead and continuation classifiers per RFC 3629 §4.
     * These cover the disjoint top-bit patterns only — overlong / surrogate /
     * above-Unicode validity is composed on top by callers that need it. */

    static inline bool SolidSyslogUtf8_IsAsciiByte(char byte)
    {
        return ((unsigned char) byte & 0x80U) == 0U;
    }

    static inline bool SolidSyslogUtf8_IsContinuationByte(char byte)
    {
        return ((unsigned char) byte & 0xC0U) == 0x80U;
    }

    static inline bool SolidSyslogUtf8_IsTwoByteLead(char byte)
    {
        return ((unsigned char) byte & 0xE0U) == 0xC0U;
    }

    static inline bool SolidSyslogUtf8_IsThreeByteLead(char byte)
    {
        return ((unsigned char) byte & 0xF0U) == 0xE0U;
    }

    static inline bool SolidSyslogUtf8_IsFourByteLead(char byte)
    {
        return ((unsigned char) byte & 0xF8U) == 0xF0U;
    }

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGUTF8_H */
