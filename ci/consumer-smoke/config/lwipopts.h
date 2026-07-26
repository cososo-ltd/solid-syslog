/* lwIP options for the consumer smoke test (S30.05).
 *
 * A real integrator brings their own lwipopts.h, and this is the smallest one
 * that lets the SolidSyslog LwipRaw adapter sources compile: NO_SYS=1 so no
 * sys_arch port is needed, the three APIs the adapters call, and DNS so the
 * SolidSyslogLwipRawDnsResolver component has lwip/dns.h to compile against.
 *
 * Nothing here is ever run. The lane cross-builds a static library to prove a
 * scrubbed-environment consumer can select and LINK the packs, which is the
 * failure mode a target-exists assertion cannot see. */
#ifndef SOLIDSYSLOG_CONSUMER_SMOKE_LWIPOPTS_H
#define SOLIDSYSLOG_CONSUMER_SMOKE_LWIPOPTS_H

#define NO_SYS 1
#define LWIP_RAW 1
#define LWIP_UDP 1
#define LWIP_TCP 1
#define LWIP_DNS 1
#define LWIP_NETCONN 0
#define LWIP_SOCKET 0
#define LWIP_DHCP 0
#define LWIP_ARP 0
#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define MEM_LIBC_MALLOC 1
#define MEMP_MEM_MALLOC 1
#define SYS_LIGHTWEIGHT_PROT 0

#endif /* SOLIDSYSLOG_CONSUMER_SMOKE_LWIPOPTS_H */
