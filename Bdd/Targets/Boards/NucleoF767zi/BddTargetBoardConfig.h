#ifndef BDD_TARGET_BOARD_CONFIG_H
#define BDD_TARGET_BOARD_CONFIG_H

/* What a target's configuration headers need to know about the board at
 * compile time, as macros because the kernel configuration reads them in the
 * preprocessor. Every board directory under Bdd/Targets/Boards/ has a header
 * of this name.
 *
 * This one is ST's NUCLEO-F767ZI, a Cortex-M7. Its Board.cmake names the
 * targets it supports. */

/* The board's network addresses, each as the octets a stack's initialiser
 * takes: BDD_TARGET_BOARD_IP_OCTETS, _NETMASK_OCTETS, _GATEWAY_OCTETS,
 * _MAC_OCTETS, and the syslog oracle as BDD_TARGET_BOARD_ORACLE_OCTETS and as
 * text, BDD_TARGET_BOARD_ORACLE_ADDRESS. BDD_TARGET_BOARD_DNS_OCTETS is the
 * gateway again: the board is given no name server, and no target on it
 * resolves a name. The addresses depend on the network the board is plugged
 * into, so the configure writes them - see Board.cmake. */
#include "BddTargetBoardAddresses.h"

/* The core clock, which the kernel divides down to its tick: Board_Init brings
 * the part to 216 MHz. */
#define BDD_TARGET_BOARD_CPU_CLOCK_HZ 216000000

/* The NVIC implements the top four bits of each 8-bit priority field. */
#define BDD_TARGET_BOARD_NVIC_PRIO_BITS 4

/* The disk, as BddTargetBoardDisk.h presents it: the first 8 MiB of the
 * MicroSD card, the size of the QEMU machine's disk, so a filesystem is laid
 * out the same on both. The LittleFS block device on the flash takes its size
 * from the same two figures - see BddTargetLittleFsDisk.h - and so is the
 * first 8 MiB of the part. */
#define BDD_TARGET_BOARD_DISK_SECTOR_SIZE 512
#define BDD_TARGET_BOARD_DISK_SECTOR_COUNT 16384

#endif /* BDD_TARGET_BOARD_CONFIG_H */
