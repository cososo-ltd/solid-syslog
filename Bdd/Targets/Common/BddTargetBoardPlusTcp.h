#ifndef BDD_TARGET_BOARD_PLUS_TCP_H
#define BDD_TARGET_BOARD_PLUS_TCP_H

#include <FreeRTOS.h>

#include <FreeRTOS_IP.h>
#include <FreeRTOS_Routing.h>

/* The board's Ethernet controller as a FreeRTOS-Plus-TCP network interface.
 * Part of the board seam - see BddTargetBoard.h - and defined in the board's
 * directory.
 *
 * Fills the descriptor for the board's one controller and returns it, leaving
 * the controller's interrupt at a priority from which its handler may call
 * the kernel. Call before FreeRTOS_IPInit_Multi; the descriptor has to
 * outlive the IP stack. */
NetworkInterface_t* BddTargetBoardPlusTcp_FillInterface(NetworkInterface_t* networkInterface);

#endif /* BDD_TARGET_BOARD_PLUS_TCP_H */
