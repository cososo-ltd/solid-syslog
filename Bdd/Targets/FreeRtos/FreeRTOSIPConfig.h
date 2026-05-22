#ifndef FREERTOS_IP_CONFIG_H
#define FREERTOS_IP_CONFIG_H

/* FreeRTOS-Plus-TCP configuration for the QEMU mps2-an385 BDD target. UDP +
 * TCP, IPv4 static address, no DHCP / LLMNR / NBNS / IPv6. UDP was brought
 * up in S08.03; S08.09 enables TCP for the SolidSyslogFreeRtosTcpStream
 * adapter so the SwitchingSender's TCP branch can land its frames on the
 * syslog-ng oracle alongside the existing UDP scenarios. S08.08 enables
 * DNS (ipconfigUSE_DNS) so SolidSyslogFreeRtosResolver can wrap
 * FreeRTOS_getaddrinfo and resolve hostnames via the slirp DNS forwarder
 * at 10.0.2.3; the cache is enabled to avoid re-querying for back-to-back
 * Resolve calls during scenario steps. */

#define ipconfigBYTE_ORDER pdFREERTOS_LITTLE_ENDIAN

#define ipconfigUSE_IPv4 1
#define ipconfigUSE_IPv6 0
#define ipconfigUSE_RA 0
#define ipconfigUSE_TCP 1
#define ipconfigTCP_KEEP_ALIVE 1
#define ipconfigTCP_KEEP_ALIVE_INTERVAL 10
#define ipconfigUSE_DHCP 0
#define ipconfigUSE_DHCPv6 0
#define ipconfigUSE_DHCP_HOOK 0
#define ipconfigUSE_DNS 1
#define ipconfigUSE_DNS_CACHE 1
#define ipconfigUSE_LLMNR 0
#define ipconfigUSE_NBNS 0
#define ipconfigUSE_NETWORK_EVENT_HOOK 1

#define ipconfigDRIVER_INCLUDED_RX_IP_CHECKSUM 0
#define ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM 0

#define ipconfigALLOW_SOCKET_SEND_WITHOUT_BIND 1
#define ipconfigINCLUDE_FULL_INET_ADDR 1
#define ipconfigSUPPORT_OUTGOING_PINGS 0
#define ipconfigSUPPORT_SELECT_FUNCTION 0
#define ipconfigFILTER_OUT_NON_ETHERNET_II_FRAMES 1

/* Doubled from 8 — TCP needs descriptors for its retransmit window in
 * addition to the in-flight UDP frames, and 8 is already tight on the BDD
 * scenarios. The extra ~3 KB of .bss is trivial against mps2-an385 SRAM. */
#define ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS 16
#define ipconfigEVENT_QUEUE_LENGTH (ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS + 5)
#define ipconfigNETWORK_MTU 1500

#define ipconfigSOCK_DEFAULT_RECEIVE_BLOCK_TIME (5000U)
#define ipconfigSOCK_DEFAULT_SEND_BLOCK_TIME (5000U)
#define ipconfigUDP_MAX_SEND_BLOCK_TIME_TICKS (5000U / portTICK_PERIOD_MS)
#define ipconfigUDP_TIME_TO_LIVE 128

#define ipconfigARP_CACHE_ENTRIES 6
#define ipconfigMAX_ARP_RETRANSMISSIONS 5
#define ipconfigMAX_ARP_AGE 150

#define ipconfigIP_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#define ipconfigIP_TASK_STACK_SIZE_WORDS (configMINIMAL_STACK_SIZE * 5U)

#define ipconfigHAS_PRINTF 0
#define ipconfigHAS_DEBUG_PRINTF 0

#endif /* FREERTOS_IP_CONFIG_H */
