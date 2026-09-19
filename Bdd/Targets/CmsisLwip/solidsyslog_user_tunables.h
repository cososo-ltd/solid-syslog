#ifndef SOLIDSYSLOG_USER_TUNABLES_H
#define SOLIDSYSLOG_USER_TUNABLES_H

/* FreeRTOS + lwIP BDD-target tuning. Mirrors
 * Bdd/Targets/FreeRtos/solidsyslog_user_tunables.h - the lwIP target runs the
 * same Cortex-M3 under the same FreeRtosMutex constraint.
 *
 * The target creates two FreeRtosMutex instances: `bufferMutex` (gating the
 * CircularBuffer producers against the Service-task drain) and
 * `lifecycleMutex` (serialising SolidSyslog_Service against teardown). The
 * library default of 1 would silently fall the second Create back to
 * NullMutex - Lock/Unlock would become no-ops. */
#define SOLIDSYSLOG_MUTEX_POOL_SIZE 2U

#endif /* SOLIDSYSLOG_USER_TUNABLES_H */
