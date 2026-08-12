/* Host-side test config for lwIP. Trimmed to the minimum that lets the
 * SolidSyslogLwipRaw* wrappers compile and exercise their unit tests:
 *  - NO_SYS=1 - no OS abstraction (LwipRaw is OS-agnostic by construction)
 *  - LWIP_RAW / UDP / TCP on - the three APIs the wrappers use
 *  - LWIP_DNS on - exposes lwip/dns.h (dns_gethostbyname, dns_found_callback)
 *    so the LwipDnsFake can define them for SolidSyslogLwipRawDnsResolver tests.
 *    No real dns.c is compiled into any host test exe; the fake supplies the
 *    symbols. The numeric SolidSyslogLwipRawResolver ignores DNS entirely.
 *  - MEM_LIBC_MALLOC=1 - host tests can use libc; production wrappers
 *    must not depend on lwIP's mem pool. */
#ifndef SOLIDSYSLOG_TEST_LWIPOPTS_H
#define SOLIDSYSLOG_TEST_LWIPOPTS_H

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

#endif /* SOLIDSYSLOG_TEST_LWIPOPTS_H */
