#ifndef SOLIDSYSLOGFORMATTER_H
#define SOLIDSYSLOGFORMATTER_H

#include "ExternC.h"

#include <stddef.h>
#include <stdint.h>

EXTERN_C_BEGIN

    typedef size_t SolidSyslogFormatterStorage;

    enum
    {
        SOLIDSYSLOG_FORMATTER_OVERHEAD = 2U
    };

/* NOLINTBEGIN(cppcoreguidelines-macro-usage) — worst-case output sizing
   macros: must be preprocessor-visible array-size const-expressions, so a
   static const / constexpr cannot replace them. Now that this header is
   library-private (Core/Source) the root .clang-tidy governs it and enables
   the rule; the Core/Interface tier-wide disable no longer covers it. Matches
   the SolidSyslogMacros.h idiom. */
#define SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(bufferSize) \
    (SOLIDSYSLOG_FORMATTER_OVERHEAD +                  \
     (((bufferSize) + sizeof(SolidSyslogFormatterStorage) - 1U) / sizeof(SolidSyslogFormatterStorage)))

#define SOLIDSYSLOG_ESCAPED_MAX_SIZE(maxDecodedLength) (2U * (maxDecodedLength))
    /* NOLINTEND(cppcoreguidelines-macro-usage) */

    struct SolidSyslogFormatter;

    static inline struct SolidSyslogFormatter* SolidSyslogFormatter_FromStorage(SolidSyslogFormatterStorage * storage)
    {
        return (struct SolidSyslogFormatter*) storage;
    }

    struct SolidSyslogFormatter* SolidSyslogFormatter_Create(SolidSyslogFormatterStorage * storage, size_t bufferSize);
    void SolidSyslogFormatter_AsciiCharacter(struct SolidSyslogFormatter * formatter, char value);
    void SolidSyslogFormatter_Bom(struct SolidSyslogFormatter * formatter);
    void SolidSyslogFormatter_NilValue(struct SolidSyslogFormatter * formatter);
    void SolidSyslogFormatter_BoundedString(
        struct SolidSyslogFormatter * formatter,
        const char* source,
        size_t maxLength
    );
    void SolidSyslogFormatter_EscapedString(
        struct SolidSyslogFormatter * formatter,
        const char* source,
        size_t maxDecodedLength
    );
    void SolidSyslogFormatter_PrintUsAsciiString(
        struct SolidSyslogFormatter * formatter,
        const char* source,
        size_t maxLength
    );
    void SolidSyslogFormatter_Uint32(struct SolidSyslogFormatter * formatter, uint32_t value);
    void SolidSyslogFormatter_TwoDigit(struct SolidSyslogFormatter * formatter, uint32_t value);
    void SolidSyslogFormatter_FourDigit(struct SolidSyslogFormatter * formatter, uint32_t value);
    void SolidSyslogFormatter_SixDigit(struct SolidSyslogFormatter * formatter, uint32_t value);
    /* Returns a pointer to the formatted bytes. The buffer is NUL-terminated for
     * convenience but the content is not a C string — UTF-8 content may contain
     * embedded NUL (U+0000), and a truncated multi-byte tail is masked with NULs
     * so strlen stops before any invalid UTF-8. _Length reports the raw byte
     * count (independent of the trim); bytes in [strlen(_AsFormattedBuffer),
     * _Length) are guaranteed zero-padding. */
    const char* SolidSyslogFormatter_AsFormattedBuffer(struct SolidSyslogFormatter * formatter);
    size_t SolidSyslogFormatter_Length(const struct SolidSyslogFormatter* formatter);

EXTERN_C_END

#endif /* SOLIDSYSLOGFORMATTER_H */
