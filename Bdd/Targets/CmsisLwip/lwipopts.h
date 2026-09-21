/* lwIP options for the QEMU mps2-an385 CMSIS-RTOS2 + lwIP BDD target.
 *
 * NO_SYS=0: lwIP runs its own "tcpip" thread and a LAN9118 netif
 * (netif/EthernetIf.c) drives the wire. The sequential netconn and socket
 * layers are ON, because the SolidSyslog LwipSocket adapters call the
 * lwip_-prefixed socket entry points from whichever task they run on; the
 * tcpip thread serves them, RX delivery (tcpip_input) and the timers. No
 * marshal seam - that is what this tier buys over the Raw API one, which the
 * sibling lwIP target still proves.
 *
 * Memory is lwIP-pool managed (no libc malloc) so the footprint is the static,
 * embedded-realistic shape an integrator ships - not the host-test shortcut
 * (MEM_LIBC_MALLOC) in Tests/Support/LwipFakes/Interface/lwipopts.h. */
#ifndef SOLIDSYSLOG_FREERTOS_LWIP_LWIPOPTS_H
#define SOLIDSYSLOG_FREERTOS_LWIP_LWIPOPTS_H

/* --- OS abstraction (NO_SYS=0: tcpip thread + FreeRTOS sys_arch) ------- */
#define NO_SYS 0
#define SYS_LIGHTWEIGHT_PROT 1
#define LWIP_TCPIP_CORE_LOCKING 1
/* Sockets, and netconn with it - lwIP builds the socket layer on netconn.
 * LWIP_SOCKET_SELECT is left at its default of 1; the stream's bounded connect
 * is what needs it. */
#define LWIP_NETCONN 1
#define LWIP_SOCKET 1

/* tcpip thread. Priorities are numeric here: lwipopts.h is processed via
 * lwip/opt.h before any FreeRTOS header, so configMAX_PRIORITIES (56, which
 * the CMSIS-RTOS2 wrapper fixes) is not visible - TCPIP_THREAD_PRIO 55 ==
 * configMAX_PRIORITIES - 1, above the LAN9118 RX task
 * (configMAX_PRIORITIES - 2 == 54, set in EthernetIf.c). Both numbers are
 * written out here and must be re-based by hand if that macro changes: the
 * tcpip thread sitting below the RX task that feeds it would let a burst
 * starve it.
 * TCPIP_THREAD_STACKSIZE is in BYTES (LWIP_FREERTOS_THREAD_STACKSIZE_IS_
 * STACKWORDS defaults to 0, so the sys_arch divides by sizeof(StackType_t)). */
/* lwIP's api/err.c maps err_t to errno; pull the E* codes from newlib's
 * <errno.h> rather than have lwIP provide its own (which would clash with
 * newlib's definitions). */
#define LWIP_ERRNO_STDINCLUDE 1

#define TCPIP_THREAD_NAME "tcpip"
#define TCPIP_THREAD_STACKSIZE 4096
#define TCPIP_THREAD_PRIO 55
#define TCPIP_MBOX_SIZE 8
#define DEFAULT_RAW_RECVMBOX_SIZE 8
#define DEFAULT_UDP_RECVMBOX_SIZE 8
#define DEFAULT_TCP_RECVMBOX_SIZE 8
#define DEFAULT_ACCEPTMBOX_SIZE 8

/* --- Protocol surface ------------------------------------------------- */
#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_RAW 1
#define LWIP_UDP 1
#define LWIP_TCP 1
#define LWIP_ARP 1
#define LWIP_DHCP 0
#define LWIP_ICMP 1
#define LWIP_IGMP 0

/* --- DNS (local hostlist only) ---------------------------------------- */
/* LWIP_DNS on so SolidSyslogLwipSocketResolver can resolve the oracle by name
 * ("syslog-ng") instead of the numeric 10.0.2.2 it was pinned to. We do NOT
 * configure a DNS server: the QEMU slirp forwarder (10.0.2.3) would resolve the
 * "syslog-ng" docker alias to a docker-bridge IP the guest has no route to -
 * only 10.0.2.2 (slirp NAT -> shared-namespace host loopback) reaches the
 * oracle. So we map the name statically via DNS_LOCAL_HOSTLIST: dns_gethostbyname
 * consults the hostlist before any server and returns ERR_OK synchronously for a
 * hit, so lwip_getaddrinfo answers without the resolve leaving the guest. The
 * over-the-wire and timeout paths are unit-tested instead - slirp cannot hand
 * the guest a reachable address for the docker alias.
 * DNS_LOCAL_HOSTLIST_INIT is expanded inside lwIP's dns.c, where
 * DNS_LOCAL_HOSTLIST_ELEM (lwip/dns.h) and IPADDR4_INIT_BYTES (lwip/ip_addr.h)
 * are in scope. */
#define LWIP_DNS 1
#define DNS_LOCAL_HOSTLIST 1
#define DNS_LOCAL_HOSTLIST_INIT {DNS_LOCAL_HOSTLIST_ELEM("syslog-ng", IPADDR4_INIT_BYTES(10, 0, 2, 2))}

/* etharp queues the first packet to a destination while ARP resolves it -
 * keep queueing on so the first UDP datagram after boot is not dropped
 * (mirrors the FreeRTOS-Plus-TCP first-packet ARP behaviour). */
#define ARP_QUEUEING 1

/* --- Memory: lwIP-managed static pools (no libc/posix heap) ----------- */
#define MEM_LIBC_MALLOC 0
#define MEMP_MEM_MALLOC 0
#define MEM_ALIGNMENT 4
#define MEM_SIZE (16 * 1024)

#define MEMP_NUM_UDP_PCB 4
#define MEMP_NUM_TCP_PCB 4
#define MEMP_NUM_TCP_PCB_LISTEN 2
#define MEMP_NUM_TCP_SEG 16
#define MEMP_NUM_PBUF 16
#define MEMP_NUM_RAW_PCB 2
#define MEMP_NUM_ARP_QUEUE 4
/* tcpip thread message pools: API calls made by the netconn layer on behalf of
 * a socket + inbound packets posted by the netif RX task via tcpip_input. */
#define MEMP_NUM_TCPIP_MSG_API 8
#define MEMP_NUM_TCPIP_MSG_INPKT 8

/* Sockets / netconn pools. Three sockets are live at once at most - the UDP
 * datagram, the plain-TCP stream, and the TCP stream under TLS - but a
 * reconnect can overlap a close, so there is headroom above that. MEMP_NUM_NETDB
 * defaults to 1 and every resolve takes one, which is enough only because
 * lwip_freeaddrinfo runs on every path the adapter can leave by. */
#define MEMP_NUM_NETCONN 6
#define MEMP_NUM_NETBUF 8
#define MEMP_NUM_SELECT_CB 4

#define PBUF_POOL_SIZE 16

/* --- TCP sizing ------------------------------------------------------- */
#define TCP_MSS 1460
#define TCP_WND (4 * TCP_MSS)
#define TCP_SND_BUF (4 * TCP_MSS)
#define TCP_QUEUE_OOSEQ 0

/* --- Diagnostics ------------------------------------------------------ */
#define LWIP_STATS 0
#define LWIP_NETIF_API 0
#define LWIP_NETIF_STATUS_CALLBACK 0
#define LWIP_NETIF_LINK_CALLBACK 0
#define CHECKSUM_GEN_IP 1
#define CHECKSUM_GEN_UDP 1
#define CHECKSUM_GEN_TCP 1
#define CHECKSUM_CHECK_IP 1
#define CHECKSUM_CHECK_UDP 1
#define CHECKSUM_CHECK_TCP 1

#endif /* SOLIDSYSLOG_FREERTOS_LWIP_LWIPOPTS_H */
