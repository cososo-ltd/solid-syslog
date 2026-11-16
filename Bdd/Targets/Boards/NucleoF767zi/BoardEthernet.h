#ifndef BOARD_ETHERNET_H
#define BOARD_ETHERNET_H

#include <stdbool.h>
#include <stdint.h>

/* The board's part of the NUCLEO-F767ZI's Ethernet MAC and its on-board
 * LAN8742A PHY, common to every network stack.
 *
 * BoardEthernet.c defines HAL_ETH_MspInit, which ST's HAL Ethernet driver
 * calls from HAL_ETH_Init. It sets up the MAC's clock, its RMII pins and its
 * interrupt, at a priority from which the handler may call the kernel, and
 * puts the PHY in its negotiating mode and resets it before the MAC is reset.
 * The HAL calls it only the first time a MAC is initialised: repeating
 * HAL_ETH_Init for a MAC that did not start repeats the MAC's reset but not
 * the PHY's restart.
 *
 * The two functions below access the PHY's registers over the MAC's
 * management bus, with the signatures EthernetPhy.h takes. context is the
 * MAC's ETH_HandleTypeDef. Each returns false when the access did not
 * complete. */
bool BoardEthernet_ReadPhy(void* context, uint8_t registerNumber, uint16_t* value);
bool BoardEthernet_WritePhy(void* context, uint8_t registerNumber, uint16_t value);

#endif /* BOARD_ETHERNET_H */
