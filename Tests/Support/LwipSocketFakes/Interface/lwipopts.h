/* Host-side test config for lwIP's Sockets API. Trimmed to the minimum that
 * lets the SolidSyslogLwipSocket* adapters compile and exercise their unit
 * tests:
 *  - LWIP_SOCKET on, which the whole pack gates on, and LWIP_NETCONN with it
 *    because the sockets layer is built on netconn.
 *  - NO_SYS=0 - LWIP_SOCKET requires it, and it is the build shape this pack
 *    exists to serve.
 *  - LWIP_DNS on - exposes lwip/netdb.h so the fake can define
 *    lwip_getaddrinfo and lwip_freeaddrinfo for the resolver's tests. No
 *    upstream netdb.c is compiled into any host test exe; the fake supplies
 *    the symbols.
 *  - LWIP_COMPAT_SOCKETS off - the adapters call the lwip_-prefixed entry
 *    points, so the unprefixed macros would only mask a wrong call.
 *  - LWIP_IPV6 off - the pack is IPv4, and a host test that wants to prove
 *    the family refusal hands the fake an AF_INET6 result directly rather
 *    than asking lwIP to produce one.
 *  - MEM_LIBC_MALLOC=1 - host tests can use libc; production adapters must
 *    not depend on lwIP's mem pool.
 *  - LWIP_TIMEVAL_PRIVATE=0 - lwIP defines its own struct timeval for select
 *    unless told the system has one, which collides with the host's the moment
 *    a test includes anything from the standard library. Upstream's own
 *    instruction for that case is this option plus <sys/time.h> in cc.h, which
 *    the shim alongside does. A target build keeps the default. */
#ifndef SOLIDSYSLOG_TEST_LWIPSOCKET_LWIPOPTS_H
#define SOLIDSYSLOG_TEST_LWIPSOCKET_LWIPOPTS_H

#define NO_SYS 0
#define LWIP_SOCKET 1
#define LWIP_NETCONN 1
#define LWIP_COMPAT_SOCKETS 0
#define LWIP_DNS 1
#define LWIP_TCP 1
#define LWIP_UDP 1
#define LWIP_RAW 0
#define LWIP_DHCP 0
#define LWIP_ARP 0
#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define MEM_LIBC_MALLOC 1
#define MEMP_MEM_MALLOC 1
#define LWIP_TIMEVAL_PRIVATE 0
#define SYS_LIGHTWEIGHT_PROT 0

#endif /* SOLIDSYSLOG_TEST_LWIPSOCKET_LWIPOPTS_H */
