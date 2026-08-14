#ifndef SOLIDSYSLOG_USER_TUNABLES_H
#define SOLIDSYSLOG_USER_TUNABLES_H

/* FreeRTOS BDD-target tuning.
 *
 * The BDD target creates two FreeRtosMutex instances: `bufferMutex` (gating
 * the CircularBuffer producers against the Service task drain) and
 * `lifecycleMutex` (serialising SolidSyslog_Service against the
 * `set store file` rebuild path that destroys and re-creates SolidSyslog
 * mid-run). The library default of 1 would silently fall the second Create
 * back to NullMutex - Lock/Unlock would become no-ops and the rebuild path
 * could race the Service task. */
#define SOLIDSYSLOG_MUTEX_POOL_SIZE 2U

#endif /* SOLIDSYSLOG_USER_TUNABLES_H */
