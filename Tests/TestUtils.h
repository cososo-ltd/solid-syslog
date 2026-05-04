#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <stddef.h>

static inline size_t MinSize(size_t a, size_t b)
{
    return (a < b) ? a : b;
}

#ifdef __cplusplus

#include "CppUTest/TestHarness.h" // IWYU pragma: keep — only used by the C++ macros below; placed inside the __cplusplus guard so C consumers don't pull in C++ headers.

namespace CososoTesting
{
/* Expected call counts — readable names for CALLED_FUNCTION assertions.
 * Namespaced because NEVER/ONCE are common identifiers that could collide
 * at include sites. Tests opt in via `using namespace CososoTesting;`. */
enum
{
    NEVER  = 0,
    ONCE   = 1,
    TWICE  = 2,
    THRICE = 3,
};
} // namespace CososoTesting

/* Assert that a call-count expression equals an expected count.
 * Reads as: CALLED_FUNCTION(SenderFake_SendCount(inner), ONCE);
 * (with `using namespace CososoTesting;` in scope). */
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- CppUTest assertions are macro-based */
#define CALLED_FUNCTION(f, n) LONGS_EQUAL((n), (f))

#endif /* __cplusplus */

#endif /* TESTUTILS_H */
