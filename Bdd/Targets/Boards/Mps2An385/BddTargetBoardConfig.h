#ifndef BDD_TARGET_BOARD_CONFIG_H
#define BDD_TARGET_BOARD_CONFIG_H

/* Facts about the board that a target's configuration headers need at compile
 * time, as macros because the kernel configuration reads them in the
 * preprocessor. Every board directory under Bdd/Targets/Boards/ has a header
 * of this name defining the same macros, and each states its fact once, so the
 * kernel configuration and the CMSIS device header take it from here rather
 * than from each other.
 *
 * This one is the QEMU mps2-an385 machine, a Cortex-M3. */

/* The core clock, which the kernel divides down to its tick. */
#define BDD_TARGET_BOARD_CPU_CLOCK_HZ 25000000

/* The NVIC implements the top three bits of each 8-bit priority field. */
#define BDD_TARGET_BOARD_NVIC_PRIO_BITS 3

/* The network the board sits on, each address as the octets a stack's own
 * initialiser takes. QEMU's user-mode network, slirp, has no DHCP server the
 * targets rely on, so the address is the one slirp would have handed out. Its
 * gateway is routed to the machine QEMU runs on, and its DNS forwarder sits
 * beside it. */
#define BDD_TARGET_BOARD_IP_OCTETS 10U, 0U, 2U, 15U
#define BDD_TARGET_BOARD_NETMASK_OCTETS 255U, 255U, 255U, 0U
#define BDD_TARGET_BOARD_GATEWAY_OCTETS 10U, 0U, 2U, 2U
#define BDD_TARGET_BOARD_DNS_OCTETS 10U, 0U, 2U, 3U

/* A locally administered unicast hardware address. */
#define BDD_TARGET_BOARD_MAC_OCTETS 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U

/* Where the syslog oracle is reached from the board: the slirp gateway, since
 * the oracle listens on the machine QEMU runs on. The same address twice, as
 * the octets and as the text a resolver takes, because C offers no way to
 * derive one from the other. */
#define BDD_TARGET_BOARD_ORACLE_OCTETS 10U, 0U, 2U, 2U
#define BDD_TARGET_BOARD_ORACLE_ADDRESS "10.0.2.2"

/* The disk, as BddTargetBoardDisk.h presents it: 16384 sectors of 512 B, which
 * is 8 MiB. Large enough for the store-and-forward, capacity and power-cycle
 * scenarios that exercise multi-block files, and clear of the ~4085-cluster
 * FAT12/16 boundary so both FAT formatters land FAT16. */
#define BDD_TARGET_BOARD_DISK_SECTOR_SIZE 512
#define BDD_TARGET_BOARD_DISK_SECTOR_COUNT 16384

#endif /* BDD_TARGET_BOARD_CONFIG_H */
