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

#endif /* BDD_TARGET_BOARD_CONFIG_H */
