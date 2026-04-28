#ifndef SOLIDSYSLOGUTF8_H
#define SOLIDSYSLOGUTF8_H

#include "ExternC.h"

#include <stdbool.h>

EXTERN_C_BEGIN

    /* Byte-level UTF-8 lead and continuation classifiers per RFC 3629 §4.
     * These cover the disjoint top-bit patterns only — overlong / surrogate /
     * above-Unicode validity is composed on top by callers that need it. */

    static inline bool SolidSyslogUtf8_IsAsciiByte(char byte)
    {
        return (byte & 0x80) == 0;
    }

    static inline bool SolidSyslogUtf8_IsContinuationByte(char byte)
    {
        return (byte & 0xC0) == 0x80;
    }

    static inline bool SolidSyslogUtf8_IsTwoByteLead(char byte)
    {
        return (byte & 0xE0) == 0xC0;
    }

    static inline bool SolidSyslogUtf8_IsThreeByteLead(char byte)
    {
        return (byte & 0xF0) == 0xE0;
    }

    static inline bool SolidSyslogUtf8_IsFourByteLead(char byte)
    {
        return (byte & 0xF8) == 0xF0;
    }

EXTERN_C_END

#endif /* SOLIDSYSLOGUTF8_H */
