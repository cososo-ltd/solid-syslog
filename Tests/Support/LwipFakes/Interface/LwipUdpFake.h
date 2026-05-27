#ifndef LWIPUDPFAKE_H
#define LWIPUDPFAKE_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

#include "lwip/ip_addr.h"

EXTERN_C_BEGIN

    struct udp_pcb;
    struct pbuf;

    void LwipUdpFake_Reset(void);

    /* udp_new configuration */
    void LwipUdpFake_SetUdpNewFails(bool fails);

    /* udp_new spy */
    unsigned LwipUdpFake_UdpNewCallCount(void);
    struct udp_pcb* LwipUdpFake_LastUdpNewReturned(void);

    /* udp_remove spy */
    unsigned LwipUdpFake_UdpRemoveCallCount(void);
    struct udp_pcb* LwipUdpFake_LastUdpRemovePcb(void);

    /* udp_sendto configuration */
    void LwipUdpFake_SetUdpSendtoError(int8_t err);

    /* udp_sendto spy */
    unsigned LwipUdpFake_UdpSendtoCallCount(void);
    struct udp_pcb* LwipUdpFake_LastSendtoPcb(void);
    struct pbuf* LwipUdpFake_LastSendtoPbuf(void);
    const ip_addr_t* LwipUdpFake_LastSendtoIpaddr(void);
    uint16_t LwipUdpFake_LastSendtoPort(void);

    /* Allocated-but-not-yet-freed PCB count. Successful udp_new bumps it;
     * udp_remove decrements. A test that ends with a non-zero value has
     * leaked a PCB — pin this in teardown to catch leaks across the suite. */
    int LwipUdpFake_OutstandingPcbCount(void);

EXTERN_C_END

#endif /* LWIPUDPFAKE_H */
