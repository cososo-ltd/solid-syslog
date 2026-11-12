#ifndef MPS2_AN385_H
#define MPS2_AN385_H

/* Facts about the QEMU mps2-an385 machine, a Cortex-M3, that more than one
 * layer of a target needs. Each states the chip once, so the kernel
 * configuration and the CMSIS device header take it from here rather than
 * from each other. */

/* The NVIC implements the top three bits of each 8-bit priority field. */
#define MPS2_AN385_NVIC_PRIO_BITS 3

#endif /* MPS2_AN385_H */
