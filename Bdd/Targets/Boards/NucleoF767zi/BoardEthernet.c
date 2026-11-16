/* What the NUCLEO-F767ZI's Ethernet MAC needs from the board before ST's HAL
 * driver can start it - see BoardEthernet.h. It is the same under every
 * network stack, so it is here and not in a stack's interface.
 *
 * Not host-TDD'd: this file exists only in the cross build, and what it does
 * is drive the part. The target checklist in the board's README.md is what
 * proves it, on the board. */

#include "BoardEthernet.h"

#include "EthernetPhy.h"

#include "FreeRTOS.h"

#include "stm32f7xx_hal.h"

#include <string.h>
#include <unistd.h>

enum
{
    /* The on-board PHY is strapped to address 0. */
    BOARD_ETHERNET_PHY_ADDRESS = 0,

    /* The interrupt wakes a task, so it has to sit at or below the ceiling
     * the kernel sets for interrupts that call it. */
    BOARD_ETHERNET_INTERRUPT_PRIORITY = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
};

static void BoardEthernet_ConfigurePins(void);
static void BoardEthernet_RestartPhy(ETH_HandleTypeDef* ethernet);

/* The HAL calls this from HAL_ETH_Init for the clocks, pins and interrupt,
 * which are the board's. The name and the parameter are ST's. */
void HAL_ETH_MspInit(ETH_HandleTypeDef* heth) // NOLINT(readability-identifier-naming) -- ST's name
{
    __HAL_RCC_ETH_CLK_ENABLE();
    BoardEthernet_ConfigurePins();
    HAL_NVIC_SetPriority(ETH_IRQn, (uint32_t) BOARD_ETHERNET_INTERRUPT_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
    BoardEthernet_RestartPhy(heth);
}

/* RMII, as the board's README.md has it in its pin plan. */
static void BoardEthernet_ConfigurePins(void)
{
    GPIO_InitTypeDef pins = {0};
    pins.Mode = GPIO_MODE_AF_PP;
    pins.Pull = GPIO_NOPULL;
    pins.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    pins.Alternate = GPIO_AF11_ETH;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    pins.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7; /* REF_CLK, MDIO, CRS_DV */
    HAL_GPIO_Init(GPIOA, &pins);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    pins.Pin = GPIO_PIN_13; /* TXD1 */
    HAL_GPIO_Init(GPIOB, &pins);

    __HAL_RCC_GPIOC_CLK_ENABLE();
    pins.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5; /* MDC, RXD0, RXD1 */
    HAL_GPIO_Init(GPIOC, &pins);

    __HAL_RCC_GPIOG_CLK_ENABLE();
    pins.Pin = GPIO_PIN_11 | GPIO_PIN_13; /* TX_EN, TXD0 */
    HAL_GPIO_Init(GPIOG, &pins);
}

/* The PHY's mode is a strap it reads from pins as it leaves reset, and after
 * a reset through the debug probe it has been seen to take any of several.
 * One of them powers it down, and then it gives the MAC no reference clock,
 * without which the MAC's own reset, which HAL_ETH_Init does next, never
 * completes. So the PHY is put in its mode here, between the clocks and pins
 * the management bus needs and that reset. The bus runs on the MAC's bus
 * clock alone, and needs its divider set first: the HAL leaves it at the one
 * the part resets to, which at this core clock runs the bus faster than a PHY
 * answers. */
static void BoardEthernet_RestartPhy(ETH_HandleTypeDef* ethernet)
{
    /* One line, written whole: stdio belongs to the target's interactive
     * thread, and the console lock in _write keeps one write together. */
    static const char DID_NOT_RESTART[] = "[board] ethernet the PHY did not restart\n";
    HAL_ETH_SetMDIOClockRange(ethernet);
    if (!EthernetPhy_Restart(BoardEthernet_WritePhy, BoardEthernet_ReadPhy, ethernet))
    {
        (void) write(STDOUT_FILENO, DID_NOT_RESTART, strlen(DID_NOT_RESTART));
    }
}

bool BoardEthernet_WritePhy(void* context, uint8_t registerNumber, uint16_t value)
{
    return HAL_ETH_WritePHYRegister(context, (uint32_t) BOARD_ETHERNET_PHY_ADDRESS, registerNumber, value) == HAL_OK;
}

bool BoardEthernet_ReadPhy(void* context, uint8_t registerNumber, uint16_t* value)
{
    uint32_t read = 0U;
    bool wasRead =
        HAL_ETH_ReadPHYRegister(context, (uint32_t) BOARD_ETHERNET_PHY_ADDRESS, registerNumber, &read) == HAL_OK;
    *value = (uint16_t) read;
    return wasRead;
}
