#ifndef LWIPTCPFAKE_H
#define LWIPTCPFAKE_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

#include "lwip/arch.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"

EXTERN_C_BEGIN

    void LwipTcpFake_Reset(void);

    /* tcp_new configuration */
    void LwipTcpFake_SetTcpNewFails(bool fails);

    /* tcp_new spy */
    unsigned LwipTcpFake_TcpNewCallCount(void);
    struct tcp_pcb* LwipTcpFake_LastTcpNewReturned(void);

    /* tcp_arg spy — last (pcb, arg) pair captured */
    unsigned LwipTcpFake_TcpArgCallCount(void);
    void* LwipTcpFake_LastCallbackArg(void);

    /* tcp_recv / tcp_err / tcp_sent spies — last registered callback fn captured */
    unsigned LwipTcpFake_TcpRecvCallCount(void);
    tcp_recv_fn LwipTcpFake_LastRecvFn(void);
    unsigned LwipTcpFake_TcpErrCallCount(void);
    tcp_err_fn LwipTcpFake_LastErrFn(void);
    unsigned LwipTcpFake_TcpSentCallCount(void);
    tcp_sent_fn LwipTcpFake_LastSentFn(void);

    /* tcp_connect configuration */

    /* Immediate err returned by tcp_connect itself. Default ERR_OK. */
    void LwipTcpFake_SetTcpConnectError(int8_t err);

    /* Whether tcp_connect synchronously invokes the registered connected_cb
     * before returning. Default true — happy-path successful connect. Set to
     * false for tests that drive the timeout path (no callback fires). */
    void LwipTcpFake_SetConnectCallbackFires(bool fires);

    /* The err passed to the connected_cb when it fires. Default ERR_OK. */
    void LwipTcpFake_SetConnectCallbackResult(int8_t err);

    /* tcp_connect spy */
    unsigned LwipTcpFake_TcpConnectCallCount(void);
    struct tcp_pcb* LwipTcpFake_LastConnectPcb(void);
    const ip_addr_t* LwipTcpFake_LastConnectIpaddr(void);
    uint16_t LwipTcpFake_LastConnectPort(void);
    tcp_connected_fn LwipTcpFake_LastConnectedFn(void);

    /* tcp_close configuration */
    void LwipTcpFake_SetTcpCloseError(int8_t err);

    /* tcp_close spy */
    unsigned LwipTcpFake_TcpCloseCallCount(void);
    struct tcp_pcb* LwipTcpFake_LastClosePcb(void);

    /* tcp_abort spy */
    unsigned LwipTcpFake_TcpAbortCallCount(void);
    struct tcp_pcb* LwipTcpFake_LastAbortPcb(void);

    /* tcp_write configuration */
    void LwipTcpFake_SetTcpWriteError(int8_t err);

    /* tcp_write spy */
    unsigned LwipTcpFake_TcpWriteCallCount(void);
    struct tcp_pcb* LwipTcpFake_LastWritePcb(void);
    const void* LwipTcpFake_LastWriteDataptr(void);
    uint16_t LwipTcpFake_LastWriteLength(void);
    uint8_t LwipTcpFake_LastWriteApiFlags(void);

    /* tcp_output configuration */
    void LwipTcpFake_SetTcpOutputError(int8_t err);

    /* tcp_output spy */
    unsigned LwipTcpFake_TcpOutputCallCount(void);
    struct tcp_pcb* LwipTcpFake_LastOutputPcb(void);

    /* tcp_recved spy — window-update ACK after the wrapper drains bytes */
    unsigned LwipTcpFake_TcpRecvedCallCount(void);
    struct tcp_pcb* LwipTcpFake_LastRecvedPcb(void);
    uint16_t LwipTcpFake_LastRecvedLen(void);

    /* Allocated-but-not-yet-freed PCB count. Successful tcp_new bumps it;
     * tcp_close / tcp_abort decrement. The tcp_err callback releases the
     * pcb upstream — tests that fire it via LwipTcpFake_LastErrFn must call
     * LwipTcpFake_NotePcbReleasedByErr() to keep the leak invariant honest. */
    int LwipTcpFake_OutstandingPcbCount(void);
    void LwipTcpFake_NotePcbReleasedByErr(void);

EXTERN_C_END

#endif /* LWIPTCPFAKE_H */
