#ifndef SOLIDSYSLOG_USER_TUNABLES_H
#define SOLIDSYSLOG_USER_TUNABLES_H

/* FreeRTOS + lwIP BDD-target tuning. Mirrors
 * Bdd/Targets/FreeRtos/solidsyslog_user_tunables.h — the lwIP target runs the
 * same Cortex-M3 with the same task-stack and FreeRtosMutex constraints.
 *
 * SolidSyslog_Log builds a Formatter and working buffer of
 * SOLIDSYSLOG_MAX_MESSAGE_SIZE on the caller's stack, so dropping from 2048
 * (the RFC 5424 section 6.1 SHOULD value) to 512 (the library's pre-S12.12
 * default) reclaims ~4.5KB per call — material on a Cortex-M3. RFC 5424
 * receivers must still accept 480 bytes, which this exceeds. The
 * message-size-1500 BDD scenarios are tag-gated off at runtime by the
 * tunable-driven check in Bdd/features/environment.py. */
#define SOLIDSYSLOG_MAX_MESSAGE_SIZE 512

/* The target creates two FreeRtosMutex instances: `bufferMutex` (gating the
 * CircularBuffer producers against the Service-task drain) and
 * `lifecycleMutex` (serialising SolidSyslog_Service against teardown). The
 * library default of 1 would silently fall the second Create back to
 * NullMutex — Lock/Unlock would become no-ops. */
#define SOLIDSYSLOG_MUTEX_POOL_SIZE 2U

#endif /* SOLIDSYSLOG_USER_TUNABLES_H */
