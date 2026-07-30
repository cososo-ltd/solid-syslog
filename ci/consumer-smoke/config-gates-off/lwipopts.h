/* The counter of ../config/lwipopts.h: every option a SolidSyslog adapter gates
 * on is off, so the gated-out path gets built. Compiled, never run. */
#ifndef SOLIDSYSLOG_CONSUMER_SMOKE_GATES_OFF_LWIPOPTS_H
#define SOLIDSYSLOG_CONSUMER_SMOKE_GATES_OFF_LWIPOPTS_H

#define NO_SYS 1
#define LWIP_RAW 1
#define LWIP_UDP 0
#define LWIP_TCP 0
#define LWIP_DNS 0
#define LWIP_NETCONN 0
#define LWIP_SOCKET 0
#define LWIP_DHCP 0
#define LWIP_ARP 0
#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define MEM_LIBC_MALLOC 1
#define MEMP_MEM_MALLOC 1
#define SYS_LIGHTWEIGHT_PROT 0

#endif /* SOLIDSYSLOG_CONSUMER_SMOKE_GATES_OFF_LWIPOPTS_H */
