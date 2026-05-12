/* Test fixture: drives a smaller-than-default SOLIDSYSLOG_MAX_MESSAGE_SIZE
 * via the SOLIDSYSLOG_USER_TUNABLES_FILE override mechanism. Wired in by
 * the `tunable-override-debug` CMake preset to prove that the user-config
 * override path actually flows through to the compiler. */
#define SOLIDSYSLOG_MAX_MESSAGE_SIZE 512
