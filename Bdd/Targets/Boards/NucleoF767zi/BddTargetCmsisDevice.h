#ifndef BDD_TARGET_CMSIS_DEVICE_H
#define BDD_TARGET_CMSIS_DEVICE_H

/* The CMSIS device header for the NUCLEO-F767ZI. CMSIS-FreeRTOS includes it
 * through CMSIS_device_header, which the CMSIS-RTOS2 target's kernel half
 * defines for the wrapper's source. It is ST's own device header, which names
 * the part's interrupts and the core's features and then includes the core's
 * header; the build's STM32F767xx definition selects the part in it. */

#include "stm32f7xx.h"

#endif /* BDD_TARGET_CMSIS_DEVICE_H */
