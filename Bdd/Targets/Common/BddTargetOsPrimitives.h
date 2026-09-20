#ifndef BDD_TARGET_OS_PRIMITIVES_H
#define BDD_TARGET_OS_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>

/* The OS seam for the QEMU BDD targets.
 *
 * The shared pipeline and the mbedTLS TLS senders are compiled by every QEMU
 * target, and each target decides which OS API its own code speaks. The build
 * selects exactly one implementation of this header - the way it already
 * selects one BddTargetTlsSender_* - so no shared source names a kernel.
 *
 * Everything here is in OS-neutral units, bytes and milliseconds, so a caller
 * never converts from some kernel's own tick period or stack word. */

struct SolidSyslogMutex;

/* The mutexes the shared pipeline needs. An OS whose mutexes make the caller
 * supply control-block storage declares one block per slot, so the count
 * belongs here rather than in a running total kept by the implementation. */
enum
{
    BDD_TARGET_MUTEX_BUFFER,
    BDD_TARGET_MUTEX_LIFECYCLE,
    BDD_TARGET_MUTEX_COUNT
};

/* Create the mutex for one slot, which is created at most once. Falls back to
 * the shared Null mutex as every SolidSyslog pool class does, so the caller
 * needs no NULL check. */
struct SolidSyslogMutex* BddTargetOsPrimitives_CreateMutex(unsigned slot);
void BddTargetOsPrimitives_DestroyMutex(struct SolidSyslogMutex* mutex);

/* The SolidSyslogSysUpTimeFunction from this target's OS pack, for the MetaSd. */
uint32_t BddTargetOsPrimitives_GetSysUpTime(void);

/* An opaque thread identity. Only ever tested against NULL and handed back, so
 * no kernel's own handle type reaches a caller. */
typedef void* BddTargetThread;

/* Start a thread, with its stack size in BYTES. There is no priority
 * parameter: both pipeline threads run one level above idle, and the two OS
 * APIs number their priorities differently enough that a caller-supplied
 * number would not mean the same thing on both. */
bool BddTargetOsPrimitives_Spawn(void (*entry)(void* argument), const char* name, uint32_t stackBytes);

/* Hand control to the scheduler. Does not return. */
void BddTargetOsPrimitives_StartScheduler(void);

BddTargetThread BddTargetOsPrimitives_CurrentThread(void);

/* End the calling thread. Does not return. */
void BddTargetOsPrimitives_ExitThread(void);

/* Peak unused stack in bytes, or zero for a thread that never started. Ask for
 * the calling thread with BddTargetOsPrimitives_CurrentThread(). */
uint32_t BddTargetOsPrimitives_StackHeadroomBytes(BddTargetThread thread);

/* The teardown handshake: the Service thread signals the instant before it
 * exits, and the thread tearing down waits. False when the wait timed out. */
void BddTargetOsPrimitives_Notify(BddTargetThread thread);
bool BddTargetOsPrimitives_WaitForNotify(uint32_t timeoutMilliseconds);

/* Block the calling thread. A non-zero request shorter than one tick still
 * blocks for a tick rather than merely yielding. */
void BddTargetOsPrimitives_Sleep(int milliseconds);

/* Monotonic milliseconds since the scheduler started. 64-bit so that it does
 * not wrap earlier than the kernel's own tick counter would. */
uint64_t BddTargetOsPrimitives_UptimeMilliseconds(void);

#endif /* BDD_TARGET_OS_PRIMITIVES_H */
