/* HAL configuration for the NUCLEO-F767ZI board support.
 *
 * ST's HAL reads this file by name from every one of its sources; the driver
 * tree ships a template and expects the project to supply the real thing.
 * Only the modules the board support and its Ethernet interface use are
 * enabled, with the two those modules read in turn: the clock setup sets the
 * flash latency, and the UART handle carries DMA handles. Each module enabled here has its source listed
 * in CMakeLists.txt, and the two lists are kept in step by hand. */
#ifndef STM32F7XX_HAL_CONF_H
#define STM32F7XX_HAL_CONF_H

#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_ETH_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

/* The board has no crystal fitted for HSE. The on-board ST-LINK supplies its
 * 8 MHz clock on MCO instead, so HSE runs in bypass at that frequency. */
#define HSE_VALUE 8000000U
#define HSE_STARTUP_TIMEOUT 100U
#define HSI_VALUE 16000000U
#define LSI_VALUE 32000U
#define LSE_VALUE 32768U
#define LSE_STARTUP_TIMEOUT 5000U
#define EXTERNAL_CLOCK_VALUE 12288000U

#define VDD_VALUE 3300U
#define TICK_INT_PRIORITY 0x0FU
#define USE_RTOS 0U
#define PREFETCH_ENABLE 1U
#define ART_ACCELERATOR_ENABLE 1U

#define USE_HAL_UART_REGISTER_CALLBACKS 0U
#define USE_HAL_ETH_REGISTER_CALLBACKS 0U

/* How long the Ethernet driver waits for the PHY to answer on the management
 * bus, in milliseconds. An answer takes well under one. */
#define PHY_READ_TO 100U
#define PHY_WRITE_TO 100U

#include "stm32f7xx_hal_rcc.h"
#include "stm32f7xx_hal_gpio.h"
#include "stm32f7xx_hal_dma.h"
#include "stm32f7xx_hal_cortex.h"
#include "stm32f7xx_hal_flash.h"
#include "stm32f7xx_hal_pwr.h"
#include "stm32f7xx_hal_uart.h"
#include "stm32f7xx_hal_eth.h"

/* The HAL checks its arguments through this; the board support passes it
 * constants, so the check is compiled out. */
#define assert_param(expr) ((void) 0U)

#endif
