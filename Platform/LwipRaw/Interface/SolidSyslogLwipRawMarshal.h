/** @file
 *  The marshal seam that pins the library's lwIP Raw API calls to the
 *  core-owning thread.
 *
 *  Every lwIP call the Datagram and TcpStream adapters make is routed through a
 *  single marshal hop so an integrator can run those calls on the thread that
 *  owns the lwIP core. (The numeric Resolver's ipaddr_aton parse touches no core
 *  state and is intentionally not marshalled.)
 *
 *  - NO_SYS=1 (bare metal, no RTOS): the default direct-call marshal is correct
 *    — one execution context, no core to protect.
 *  - NO_SYS=0 (RTOS with a tcpip thread): the integrator installs a marshal that
 *    hops onto that thread — e.g. tcpip_callback_with_block, or a
 *    LOCK_TCPIP_CORE / UNLOCK_TCPIP_CORE pair.
 *
 *  The marshal MUST invoke its callback synchronously, before it returns: the
 *  wrapper reads results the callback writes immediately after the hop, so an
 *  asynchronous marshal is caller error. tcpip_callback_with_block(.., block=1)
 *  honours this; a bare tcpip_callback(..) does not. See docs/platforms/lwipraw/setup.md. */
#ifndef SOLIDSYSLOGLWIPRAWMARSHAL_H
#define SOLIDSYSLOGLWIPRAWMARSHAL_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** The work the marshal must run on the core-owning thread. */
    typedef void (*SolidSyslogLwipRawCallback)(void* context);
    /** Runs callback(context) on the lwIP core thread; must return only after it
     *  has completed (see the file overview). */
    typedef void (*SolidSyslogLwipRawMarshalFunction)(SolidSyslogLwipRawCallback callback, void* context);

    /** Installs the process-global marshal. One lwIP instance and one tcpip
     *  thread per process means a single global slot suffices — same shape as
     *  SolidSyslog_SetErrorHandler. NULL restores the direct-call default.
     *  Intended for setup-time configuration; not synchronised with concurrent
     *  installs. */
    void SolidSyslogLwipRaw_SetMarshal(SolidSyslogLwipRawMarshalFunction marshal);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWMARSHAL_H */
