/* Test fixture: drives override values via the SOLIDSYSLOG_USER_TUNABLES_FILE
 * mechanism. Wired in by the `tunable-override-debug` CMake preset to
 * prove that the user-config override path actually flows through to the
 * compiler.
 *
 * - SOLIDSYSLOG_MAX_MESSAGE_SIZE smaller than the default proves the
 *   message-size knob.
 * - SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE bumped to 2 enables
 *   `EachPooledHandleHasIsolatedQueue` in SolidSyslogPosixMessageQueueBufferTest
 *   to exercise the slot-indexed queue-name discriminator (CodeRabbit
 *   feedback on PR #407).
 * - SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE bumped to 2 enables
 *   `TwoCountersFromPoolAreIndependent` in SolidSyslogAtomicCounterContractTest
 *   to exercise two concurrent counters drawn from the same pool.
 * - SOLIDSYSLOG_POOL_SIZE bumped to 3 lets the SolidSyslogPool fixture's
 *   FillPool walk three slots and exercise multi-instance Create/Destroy. */
#define SOLIDSYSLOG_MAX_MESSAGE_SIZE 512
#define SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE 2U
#define SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE 2U
#define SOLIDSYSLOG_POOL_SIZE 3U
