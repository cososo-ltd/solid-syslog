#include "LwipTcpFake.h"

#include <stddef.h>

#include "lwip/arch.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"

static unsigned tcpNewCallCount = 0;
static struct tcp_pcb fakePcb;
static struct tcp_pcb* lastTcpNewReturned = NULL;
static bool tcpNewFails = false;

static unsigned tcpArgCallCount = 0;
static void* lastCallbackArg = NULL;

static unsigned tcpRecvCallCount = 0;
static tcp_recv_fn lastRecvFn = NULL;
static unsigned tcpErrCallCount = 0;
static tcp_err_fn lastErrFn = NULL;
static unsigned tcpSentCallCount = 0;
static tcp_sent_fn lastSentFn = NULL;

static unsigned tcpConnectCallCount = 0;
static struct tcp_pcb* lastConnectPcb = NULL;
static const ip_addr_t* lastConnectIpaddr = NULL;
static u16_t lastConnectPort = 0;
static tcp_connected_fn lastConnectedFn = NULL;
static err_t tcpConnectError = ERR_OK;
static bool connectCallbackFires = true;
static err_t connectCallbackResult = ERR_OK;

static unsigned tcpCloseCallCount = 0;
static struct tcp_pcb* lastClosePcb = NULL;
static err_t tcpCloseError = ERR_OK;

static unsigned tcpAbortCallCount = 0;
static struct tcp_pcb* lastAbortPcb = NULL;

static unsigned tcpWriteCallCount = 0;
static struct tcp_pcb* lastWritePcb = NULL;
static const void* lastWriteDataptr = NULL;
static u16_t lastWriteLength = 0;
static u8_t lastWriteApiFlags = 0;
static err_t tcpWriteError = ERR_OK;

static unsigned tcpOutputCallCount = 0;
static struct tcp_pcb* lastOutputPcb = NULL;
static err_t tcpOutputError = ERR_OK;

static unsigned tcpRecvedCallCount = 0;
static struct tcp_pcb* lastRecvedPcb = NULL;
static u16_t lastRecvedLen = 0;

static int outstandingPcbCount = 0;

void LwipTcpFake_Reset(void)
{
    tcpNewCallCount = 0;
    /* Zero the fake pcb so so_options / state writes from prior tests don't
     * leak into the current one. */
    fakePcb = (struct tcp_pcb){0};
    lastTcpNewReturned = NULL;
    tcpNewFails = false;

    tcpArgCallCount = 0;
    lastCallbackArg = NULL;

    tcpRecvCallCount = 0;
    lastRecvFn = NULL;
    tcpErrCallCount = 0;
    lastErrFn = NULL;
    tcpSentCallCount = 0;
    lastSentFn = NULL;

    tcpConnectCallCount = 0;
    lastConnectPcb = NULL;
    lastConnectIpaddr = NULL;
    lastConnectPort = 0;
    lastConnectedFn = NULL;
    tcpConnectError = ERR_OK;
    connectCallbackFires = true;
    connectCallbackResult = ERR_OK;

    tcpCloseCallCount = 0;
    lastClosePcb = NULL;
    tcpCloseError = ERR_OK;

    tcpAbortCallCount = 0;
    lastAbortPcb = NULL;

    tcpWriteCallCount = 0;
    lastWritePcb = NULL;
    lastWriteDataptr = NULL;
    lastWriteLength = 0;
    lastWriteApiFlags = 0;
    tcpWriteError = ERR_OK;

    tcpOutputCallCount = 0;
    lastOutputPcb = NULL;
    tcpOutputError = ERR_OK;

    tcpRecvedCallCount = 0;
    lastRecvedPcb = NULL;
    lastRecvedLen = 0;

    outstandingPcbCount = 0;
}

void LwipTcpFake_SetTcpNewFails(bool fails)
{
    tcpNewFails = fails;
}

unsigned LwipTcpFake_TcpNewCallCount(void)
{
    return tcpNewCallCount;
}

struct tcp_pcb* LwipTcpFake_LastTcpNewReturned(void)
{
    return lastTcpNewReturned;
}

unsigned LwipTcpFake_TcpArgCallCount(void)
{
    return tcpArgCallCount;
}

void* LwipTcpFake_LastCallbackArg(void)
{
    return lastCallbackArg;
}

unsigned LwipTcpFake_TcpRecvCallCount(void)
{
    return tcpRecvCallCount;
}

tcp_recv_fn LwipTcpFake_LastRecvFn(void)
{
    return lastRecvFn;
}

unsigned LwipTcpFake_TcpErrCallCount(void)
{
    return tcpErrCallCount;
}

tcp_err_fn LwipTcpFake_LastErrFn(void)
{
    return lastErrFn;
}

unsigned LwipTcpFake_TcpSentCallCount(void)
{
    return tcpSentCallCount;
}

tcp_sent_fn LwipTcpFake_LastSentFn(void)
{
    return lastSentFn;
}

void LwipTcpFake_SetTcpConnectError(int8_t err)
{
    tcpConnectError = (err_t) err;
}

void LwipTcpFake_SetConnectCallbackFires(bool fires)
{
    connectCallbackFires = fires;
}

void LwipTcpFake_SetConnectCallbackResult(int8_t err)
{
    connectCallbackResult = (err_t) err;
}

unsigned LwipTcpFake_TcpConnectCallCount(void)
{
    return tcpConnectCallCount;
}

struct tcp_pcb* LwipTcpFake_LastConnectPcb(void)
{
    return lastConnectPcb;
}

const ip_addr_t* LwipTcpFake_LastConnectIpaddr(void)
{
    return lastConnectIpaddr;
}

uint16_t LwipTcpFake_LastConnectPort(void)
{
    return lastConnectPort;
}

tcp_connected_fn LwipTcpFake_LastConnectedFn(void)
{
    return lastConnectedFn;
}

void LwipTcpFake_SetTcpCloseError(int8_t err)
{
    tcpCloseError = (err_t) err;
}

unsigned LwipTcpFake_TcpCloseCallCount(void)
{
    return tcpCloseCallCount;
}

struct tcp_pcb* LwipTcpFake_LastClosePcb(void)
{
    return lastClosePcb;
}

unsigned LwipTcpFake_TcpAbortCallCount(void)
{
    return tcpAbortCallCount;
}

struct tcp_pcb* LwipTcpFake_LastAbortPcb(void)
{
    return lastAbortPcb;
}

void LwipTcpFake_SetTcpWriteError(int8_t err)
{
    tcpWriteError = (err_t) err;
}

unsigned LwipTcpFake_TcpWriteCallCount(void)
{
    return tcpWriteCallCount;
}

struct tcp_pcb* LwipTcpFake_LastWritePcb(void)
{
    return lastWritePcb;
}

const void* LwipTcpFake_LastWriteDataptr(void)
{
    return lastWriteDataptr;
}

uint16_t LwipTcpFake_LastWriteLength(void)
{
    return lastWriteLength;
}

uint8_t LwipTcpFake_LastWriteApiFlags(void)
{
    return lastWriteApiFlags;
}

void LwipTcpFake_SetTcpOutputError(int8_t err)
{
    tcpOutputError = (err_t) err;
}

unsigned LwipTcpFake_TcpOutputCallCount(void)
{
    return tcpOutputCallCount;
}

struct tcp_pcb* LwipTcpFake_LastOutputPcb(void)
{
    return lastOutputPcb;
}

unsigned LwipTcpFake_TcpRecvedCallCount(void)
{
    return tcpRecvedCallCount;
}

struct tcp_pcb* LwipTcpFake_LastRecvedPcb(void)
{
    return lastRecvedPcb;
}

uint16_t LwipTcpFake_LastRecvedLen(void)
{
    return lastRecvedLen;
}

int LwipTcpFake_OutstandingPcbCount(void)
{
    return outstandingPcbCount;
}

void LwipTcpFake_NotePcbReleasedByErr(void)
{
    --outstandingPcbCount;
}

/* ------------------------------------------------------------------
 * lwIP API replacements
 * ----------------------------------------------------------------*/

struct tcp_pcb* tcp_new(void)
{
    ++tcpNewCallCount;
    if (tcpNewFails)
    {
        lastTcpNewReturned = NULL;
    }
    else
    {
        lastTcpNewReturned = &fakePcb;
        ++outstandingPcbCount;
    }
    return lastTcpNewReturned;
}

void tcp_arg(struct tcp_pcb* pcb, void* arg)
{
    (void) pcb;
    ++tcpArgCallCount;
    lastCallbackArg = arg;
}

void tcp_recv(struct tcp_pcb* pcb, tcp_recv_fn recv)
{
    (void) pcb;
    ++tcpRecvCallCount;
    lastRecvFn = recv;
}

void tcp_err(struct tcp_pcb* pcb, tcp_err_fn err)
{
    (void) pcb;
    ++tcpErrCallCount;
    lastErrFn = err;
}

void tcp_sent(struct tcp_pcb* pcb, tcp_sent_fn sent)
{
    (void) pcb;
    ++tcpSentCallCount;
    lastSentFn = sent;
}

err_t tcp_connect(struct tcp_pcb* pcb, const ip_addr_t* ipaddr, u16_t port, tcp_connected_fn connected)
{
    ++tcpConnectCallCount;
    lastConnectPcb = pcb;
    lastConnectIpaddr = ipaddr;
    lastConnectPort = port;
    lastConnectedFn = connected;
    if ((tcpConnectError == ERR_OK) && connectCallbackFires && (connected != NULL))
    {
        (void) connected(lastCallbackArg, pcb, connectCallbackResult);
    }
    return tcpConnectError;
}

err_t tcp_close(struct tcp_pcb* pcb)
{
    ++tcpCloseCallCount;
    lastClosePcb = pcb;
    if (tcpCloseError == ERR_OK)
    {
        --outstandingPcbCount;
    }
    return tcpCloseError;
}

void tcp_abort(struct tcp_pcb* pcb)
{
    ++tcpAbortCallCount;
    lastAbortPcb = pcb;
    --outstandingPcbCount;
}

err_t tcp_write(struct tcp_pcb* pcb, const void* dataptr, u16_t len, u8_t apiflags)
{
    ++tcpWriteCallCount;
    lastWritePcb = pcb;
    lastWriteDataptr = dataptr;
    lastWriteLength = len;
    lastWriteApiFlags = apiflags;
    return tcpWriteError;
}

err_t tcp_output(struct tcp_pcb* pcb)
{
    ++tcpOutputCallCount;
    lastOutputPcb = pcb;
    return tcpOutputError;
}

void tcp_recved(struct tcp_pcb* pcb, u16_t len)
{
    ++tcpRecvedCallCount;
    lastRecvedPcb = pcb;
    lastRecvedLen = len;
}
