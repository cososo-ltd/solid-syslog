/* lwIP netif init callback for the QEMU mps2-an385 LAN9118 (SMSC9220)
 * Ethernet controller, under FreeRTOS + lwIP NO_SYS=0.
 *
 * Pass EthernetIf_Init to netif_add(): it programs the MAC, starts the RX
 * FreeRTOS task (driven by the IRQ-13 EthernetISR), and wires the low-level
 * output path. The matching EthernetISR (defined in EthernetIf.c) is the
 * strong override the Cortex-M3 vector table in Startup.c references at
 * IRQ 13. */
#ifndef SOLIDSYSLOG_FREERTOS_LWIP_ETHERNETIF_H
#define SOLIDSYSLOG_FREERTOS_LWIP_ETHERNETIF_H

#include "lwip/err.h"
#include "lwip/netif.h"

err_t EthernetIf_Init(struct netif* netif);

#endif /* SOLIDSYSLOG_FREERTOS_LWIP_ETHERNETIF_H */
