#ifndef BDD_TARGET_BOARD_CONFIG_H
#define BDD_TARGET_BOARD_CONFIG_H

/* Facts about the board that a target's configuration headers need at compile
 * time, as macros because the kernel configuration reads them in the
 * preprocessor. Every board directory under Bdd/Targets/Boards/ has a header
 * of this name, defining what the targets that board supports read, and each
 * states its fact once, so the kernel configuration takes it from here.
 *
 * This one is ST's NUCLEO-F767ZI, a Cortex-M7. Its Board.cmake names the
 * targets it supports. */

/* The network the board sits on, each address as the octets a stack's own
 * initialiser takes: BDD_TARGET_BOARD_IP_OCTETS, _NETMASK_OCTETS,
 * _GATEWAY_OCTETS, _MAC_OCTETS, and where the syslog oracle is reached, as
 * BDD_TARGET_BOARD_ORACLE_OCTETS and as the text a resolver takes,
 * BDD_TARGET_BOARD_ORACLE_ADDRESS. They belong to the network the board is
 * plugged into, so the configure writes them - see Board.cmake. */
#include "BddTargetBoardAddresses.h"

/* The core clock, which the kernel divides down to its tick: Board_Init brings
 * the part to 216 MHz. */
#define BDD_TARGET_BOARD_CPU_CLOCK_HZ 216000000

/* The NVIC implements the top four bits of each 8-bit priority field. */
#define BDD_TARGET_BOARD_NVIC_PRIO_BITS 4

/* The disk, as BddTargetBoardDisk.h presents it. No media is fitted, and the
 * board's disk never becomes ready, so this geometry is only what the
 * filesystem glue needs to compile. */
#define BDD_TARGET_BOARD_DISK_SECTOR_SIZE 512
#define BDD_TARGET_BOARD_DISK_SECTOR_COUNT 16384

#endif /* BDD_TARGET_BOARD_CONFIG_H */
