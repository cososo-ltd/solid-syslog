#include "LwipUdpFake.h"

#include <stddef.h>

#include "lwip/arch.h"
#include "lwip/err.h"
#include "lwip/udp.h"

static unsigned udpNewCallCount = 0;
static struct udp_pcb fakePcb;
static struct udp_pcb* lastUdpNewReturned = NULL;
static bool udpNewFails = false;

static unsigned udpRemoveCallCount = 0;
static struct udp_pcb* lastUdpRemovePcb = NULL;

static unsigned udpSendtoCallCount = 0;
static struct udp_pcb* lastSendtoPcb = NULL;
static struct pbuf* lastSendtoPbuf = NULL;
static const ip_addr_t* lastSendtoIpaddr = NULL;
static u16_t lastSendtoPort = 0;
static err_t udpSendtoError = ERR_OK;

static int outstandingPcbCount = 0;

void LwipUdpFake_Reset(void)
{
    udpNewCallCount = 0;
    lastUdpNewReturned = NULL;
    udpNewFails = false;
    udpRemoveCallCount = 0;
    lastUdpRemovePcb = NULL;
    udpSendtoCallCount = 0;
    lastSendtoPcb = NULL;
    lastSendtoPbuf = NULL;
    lastSendtoIpaddr = NULL;
    lastSendtoPort = 0;
    udpSendtoError = ERR_OK;
    outstandingPcbCount = 0;
}

void LwipUdpFake_SetUdpSendtoError(int8_t err)
{
    udpSendtoError = (err_t) err;
}

void LwipUdpFake_SetUdpNewFails(bool fails)
{
    udpNewFails = fails;
}

unsigned LwipUdpFake_UdpNewCallCount(void)
{
    return udpNewCallCount;
}

struct udp_pcb* LwipUdpFake_LastUdpNewReturned(void)
{
    return lastUdpNewReturned;
}

unsigned LwipUdpFake_UdpRemoveCallCount(void)
{
    return udpRemoveCallCount;
}

struct udp_pcb* LwipUdpFake_LastUdpRemovePcb(void)
{
    return lastUdpRemovePcb;
}

unsigned LwipUdpFake_UdpSendtoCallCount(void)
{
    return udpSendtoCallCount;
}

struct udp_pcb* LwipUdpFake_LastSendtoPcb(void)
{
    return lastSendtoPcb;
}

struct pbuf* LwipUdpFake_LastSendtoPbuf(void)
{
    return lastSendtoPbuf;
}

const ip_addr_t* LwipUdpFake_LastSendtoIpaddr(void)
{
    return lastSendtoIpaddr;
}

uint16_t LwipUdpFake_LastSendtoPort(void)
{
    return lastSendtoPort;
}

int LwipUdpFake_OutstandingPcbCount(void)
{
    return outstandingPcbCount;
}

struct udp_pcb* udp_new(void)
{
    ++udpNewCallCount;
    if (udpNewFails)
    {
        lastUdpNewReturned = NULL;
    }
    else
    {
        lastUdpNewReturned = &fakePcb;
        ++outstandingPcbCount;
    }
    return lastUdpNewReturned;
}

void udp_remove(struct udp_pcb* pcb)
{
    ++udpRemoveCallCount;
    lastUdpRemovePcb = pcb;
    --outstandingPcbCount;
}

err_t udp_sendto(struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* dst_ip, u16_t dst_port)
{
    ++udpSendtoCallCount;
    lastSendtoPcb = pcb;
    lastSendtoPbuf = p;
    lastSendtoIpaddr = dst_ip;
    lastSendtoPort = dst_port;
    return udpSendtoError;
}
