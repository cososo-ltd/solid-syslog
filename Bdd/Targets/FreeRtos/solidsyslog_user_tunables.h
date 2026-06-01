#ifndef SOLIDSYSLOG_USER_TUNABLES_H
#define SOLIDSYSLOG_USER_TUNABLES_H

/* FreeRTOS BDD-target tuning. SolidSyslog_Log allocates a Formatter and
 * working buffer of SOLIDSYSLOG_MAX_MESSAGE_SIZE on the caller's stack,
 * so dropping from 2048 (the RFC 5424 section 6.1 SHOULD value) to 512
 * (the library's pre-S12.12 default) reclaims ~4.5KB per call — material
 * on a Cortex-M3 with 4KB task stacks. RFC 5424 receivers are still
 * required to accept up to 480 bytes, which this comfortably exceeds.
 *
 * BDD impact: path-MTU clipping scenarios (udp_mtu.feature) need MAX
 * large enough to build a >1472-byte message and trigger EMSGSIZE — that
 * feature is tagged @requires_message_size_1500 and skipped at runtime
 * by the tunable-driven gate in Bdd/features/environment.py (which
 * compares the tag's threshold against this header's value via the
 * generated Bdd/features/steps/solidsyslog_tunables.py mirror). */
#define SOLIDSYSLOG_MAX_MESSAGE_SIZE 512

/* The BDD target creates two FreeRtosMutex instances: `bufferMutex` (gating
 * the CircularBuffer producers against the Service task drain) and
 * `lifecycleMutex` (serialising SolidSyslog_Service against the
 * `set store file` rebuild path that destroys and re-creates SolidSyslog
 * mid-run). The library default of 1 would silently fall the second Create
 * back to NullMutex — Lock/Unlock would become no-ops and the rebuild path
 * could race the Service task. */
#define SOLIDSYSLOG_MUTEX_POOL_SIZE 2U

#endif /* SOLIDSYSLOG_USER_TUNABLES_H */
