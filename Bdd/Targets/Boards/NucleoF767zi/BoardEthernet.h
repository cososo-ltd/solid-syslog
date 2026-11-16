#ifndef BOARD_ETHERNET_H
#define BOARD_ETHERNET_H

#include <stdbool.h>
#include <stdint.h>

/* The NUCLEO-F767ZI's Ethernet MAC and its on-board LAN8742A PHY, as far as
 * they are the board's and not a network stack's.
 *
 * BoardEthernet.c defines HAL_ETH_MspInit, which ST's HAL Ethernet driver
 * calls from HAL_ETH_Init: the MAC's clock, its RMII pins, its interrupt at a
 * priority from which the handler may call the kernel, and the PHY put in its
 * negotiating mode and reset before the MAC is. The HAL calls it the first
 * time a MAC is initialised and not again: a HAL_ETH_Init repeated for a MAC
 * that did not start repeats the MAC's reset, and not the PHY's restart.
 *
 * The two functions below reach the PHY's registers over the MAC's management
 * bus, in the shape EthernetPhy.h takes them. context is the
 * ETH_HandleTypeDef of the MAC, as a void* because that is what EthernetPhy.h
 * passes through. Each is false when the access did not complete. */
bool BoardEthernet_ReadPhy(void* context, uint8_t registerNumber, uint16_t* value);
bool BoardEthernet_WritePhy(void* context, uint8_t registerNumber, uint16_t value);

#endif /* BOARD_ETHERNET_H */
