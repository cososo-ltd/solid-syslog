#ifndef FREERTOS_IP_CONFIG_H
#define FREERTOS_IP_CONFIG_H

/* FreeRTOS-Plus-TCP configuration for the QEMU mps2-an385 bring-up smoke
 * test (slice 3b.1 of S08.03). UDP-only, IPv4 static address, no DHCP / DNS /
 * LLMNR / NBNS / TCP / IPv6. The smoke task creates a UDP socket and sends a
 * single datagram to the slirp gateway (10.0.2.2:5514). */

#define ipconfigBYTE_ORDER pdFREERTOS_LITTLE_ENDIAN

#define ipconfigUSE_IPv4 1
#define ipconfigUSE_IPv6 0
#define ipconfigUSE_RA 0
#define ipconfigUSE_TCP 0
#define ipconfigUSE_DHCP 0
#define ipconfigUSE_DHCPv6 0
#define ipconfigUSE_DHCP_HOOK 0
#define ipconfigUSE_DNS 0
#define ipconfigUSE_DNS_CACHE 0
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

#define ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS 8
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
