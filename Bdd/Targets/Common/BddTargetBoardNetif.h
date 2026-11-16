#ifndef BDD_TARGET_BOARD_NETIF_H
#define BDD_TARGET_BOARD_NETIF_H

#include "lwip/err.h"
#include "lwip/netif.h"

/* The board's Ethernet controller as an lwIP netif, for the targets that run
 * lwIP with NO_SYS=0 over FreeRTOS. Part of the board seam - see
 * BddTargetBoard.h - and defined in the board's directory.
 *
 * Pass it to netif_add() as the init callback. It sets the hardware address
 * and MTU, wires the output path, and starts whatever the board needs to hand
 * received frames to netif->input. It may block, so it runs on a task, after
 * the scheduler has started.
 *
 * The board owns the link state: a board whose link cannot drop sets
 * NETIF_FLAG_LINK_UP here, and one whose link can raises and lowers it as the
 * link changes. The target sets the interface up and leaves the link alone. */
err_t BddTargetBoardNetif_Init(struct netif* netif);

#endif /* BDD_TARGET_BOARD_NETIF_H */
