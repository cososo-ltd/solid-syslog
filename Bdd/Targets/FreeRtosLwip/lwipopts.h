/* lwIP options for the QEMU mps2-an385 FreeRTOS+lwIP link-probe target.
 *
 * S28.07 scope: prove the Platform/LwipRaw adapters (Address, Resolver,
 * Datagram, TcpStream, Marshal) cross-build and link against lwIP core for a
 * Cortex-M3 FreeRTOS target, independent of Platform/PlusTcp. There is no
 * netif driver and no QEMU run — this config only has to be self-consistent
 * enough for lwIP core + the adapters to compile and link.
 *
 * NO_SYS=1: the LwipRaw adapters are OS-agnostic and the default direct-call
 * marshal (SolidSyslogLwipRaw_SetMarshal, S28.06) is correct for a single
 * lwIP-owning context. The worked NO_SYS=0 runtime (tcpip thread + the
 * tcpip_callback marshal + a LAN9118 netif) lands with S28.09, which reuses
 * this header and flips the OS-coupling options then.
 *
 * Memory is lwIP-pool managed (no libc malloc, no MEMP_MEM_MALLOC) so the
 * footprint is the static, embedded-realistic shape an integrator ships —
 * not the host-test shortcut (MEM_LIBC_MALLOC) in
 * Tests/Support/LwipFakes/Interface/lwipopts.h. */
#ifndef SOLIDSYSLOG_FREERTOS_LWIP_LWIPOPTS_H
#define SOLIDSYSLOG_FREERTOS_LWIP_LWIPOPTS_H

/* --- OS abstraction --------------------------------------------------- */
#define NO_SYS 1
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_NETCONN 0
#define LWIP_SOCKET 0

/* --- Protocol surface ------------------------------------------------- */
#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_RAW 1
#define LWIP_UDP 1
#define LWIP_TCP 1
#define LWIP_ARP 1
#define LWIP_DHCP 0
#define LWIP_DNS 0
#define LWIP_ICMP 1
#define LWIP_IGMP 0

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

#define PBUF_POOL_SIZE 16

/* --- TCP sizing ------------------------------------------------------- */
#define TCP_MSS 1460
#define TCP_WND (4 * TCP_MSS)
#define TCP_SND_BUF (4 * TCP_MSS)
#define TCP_QUEUE_OOSEQ 0

/* --- Diagnostics: lean for the cross link probe ----------------------- */
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
