/* Host-side test config for lwIP. Trimmed to the minimum that lets the
 * SolidSyslogLwipRaw* wrappers compile and exercise their unit tests:
 *  - NO_SYS=1 — no OS abstraction (LwipRaw is OS-agnostic by construction)
 *  - LWIP_RAW / UDP / TCP on — the three APIs the wrappers use
 *  - LWIP_DNS off — sync resolver in S28.03 uses only ipaddr_aton
 *  - MEM_LIBC_MALLOC=1 — host tests can use libc; production wrappers
 *    must not depend on lwIP's mem pool. */
#ifndef SOLIDSYSLOG_TEST_LWIPOPTS_H
#define SOLIDSYSLOG_TEST_LWIPOPTS_H

// NOLINTBEGIN(cppcoreguidelines-macro-usage) -- lwIP API requires these to be #defines
#define NO_SYS 1
#define LWIP_RAW 1
#define LWIP_UDP 1
#define LWIP_TCP 1
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
// NOLINTEND(cppcoreguidelines-macro-usage)

#endif /* SOLIDSYSLOG_TEST_LWIPOPTS_H */
