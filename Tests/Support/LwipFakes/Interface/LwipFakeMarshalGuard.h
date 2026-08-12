#ifndef LWIPFAKEMARSHALGUARD_H
#define LWIPFAKEMARSHALGUARD_H

#include <stdbool.h>

#include "SolidSyslogExternC.h"
#include "SolidSyslogLwipRawMarshal.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Invariant rail proving every lwIP Raw API call the production wrappers
     * make happens inside an installed marshal. Each faked lwIP function calls
     * LWIP_REQUIRE_MARSHAL_ACTIVE() as its first statement; if the marshal is
     * not active the first breach is recorded (with the fake's call site) and
     * surfaced by LwipFakeMarshalGuard_CheckNoBreach() at teardown.
     *
     * Why record-then-check rather than fail at the call site: the fakes are
     * C, and CppUTest aborts a test by throwing / longjmp - unwinding through
     * C frames is undefined. Recording the breach and failing in the C++
     * teardown is robust, and the captured file/line still points the reader
     * straight at the offending lwIP call. */
    extern bool LwipFakeMarshalGuard_Active;

    void LwipFakeMarshalGuard_RequireActive(const char* file, int line);

    /* Resets the rail: clears any recorded breach and disarms the active flag.
     * Call from fixture setup before installing the tracking marshal. */
    void LwipFakeMarshalGuard_Reset(void);

    /* Fails the current test (at the recorded fake call site) if any lwIP API
     * was called outside a marshalled section. Call from fixture teardown. */
    void LwipFakeMarshalGuard_CheckNoBreach(void);

    /* Marshal that flips the rail flag around its callback, so production lwIP
     * calls routed through SolidSyslogLwipRaw_Marshal run with the flag active.
     * Fixtures install this via SolidSyslogLwipRaw_SetMarshal. */
    void LwipFakeMarshalGuard_TrackingMarshal(SolidSyslogLwipRawCallback callback, void* context);

SOLIDSYSLOG_EXTERN_C_END

#define LWIP_REQUIRE_MARSHAL_ACTIVE() LwipFakeMarshalGuard_RequireActive(__FILE__, __LINE__)

#endif /* LWIPFAKEMARSHALGUARD_H */
