#ifndef TESTUTILS_H
#define TESTUTILS_H

#include "CppUTest/TestHarness.h" // IWYU pragma: keep -- needed by the CALLED_* macros below

namespace CososoTesting
{
/* Expected call counts — readable names for CALLED_FUNCTION assertions.
 * Namespaced because NEVER/ONCE are common identifiers that could collide
 * at include sites. Tests opt in via `using namespace CososoTesting;`. */
enum
{
    NEVER = 0,
    ONCE = 1,
    TWICE = 2,
    THRICE = 3,
};
} // namespace CososoTesting

/* Assert that a fake/spy was called the expected number of times.
 * Token paste enforces the <name>CallCount naming rule at compile time.
 * Tests opt in to NEVER/ONCE/... via `using namespace CososoTesting;`.
 *
 * Use CALLED_FUNCTION for local static int counters in test files,
 * CALLED_FAKE for fake-library getters with no parameters, and
 * CALLED_FAKE_ON for fake-library getters that take an instance pointer.
 *
 *   CALLED_FUNCTION(handler, ONCE)
 *       -> LONGS_EQUAL(1, handlerCallCount)
 *   CALLED_FAKE(SocketFake_Send, TWICE)
 *       -> LONGS_EQUAL(2, SocketFake_SendCallCount())
 *   CALLED_FAKE_ON(SenderFake_Send, inner, ONCE)
 *       -> LONGS_EQUAL(1, SenderFake_SendCallCount(inner))
 */
#define CALLED_FUNCTION(name, count) LONGS_EQUAL((count), name##CallCount)
#define CALLED_FAKE(getter, count) LONGS_EQUAL((count), getter##CallCount())
#define CALLED_FAKE_ON(getter, instance, count) LONGS_EQUAL((count), getter##CallCount(instance))

/* Assert the last error event captured by ErrorHandlerFake matches an expected
 * (source, category, detail) triple. Prefer the portable Category — it survives
 * a backend swap — over the per-class Detail code when a portable reaction is
 * the thing under test. Use at a site that includes "ErrorHandlerFake.h". */
// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
#define CHECK_ERROR_EVENT(expectedSource, expectedCategory, expectedDetail)        \
    do                                                                             \
    {                                                                              \
        POINTERS_EQUAL((expectedSource), ErrorHandlerFake_LastSource());           \
        UNSIGNED_LONGS_EQUAL((expectedCategory), ErrorHandlerFake_LastCategory()); \
        LONGS_EQUAL((expectedDetail), ErrorHandlerFake_LastDetail());              \
    } while (0)
// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

#endif /* TESTUTILS_H */
