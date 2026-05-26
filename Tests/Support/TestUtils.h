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

#endif /* TESTUTILS_H */
