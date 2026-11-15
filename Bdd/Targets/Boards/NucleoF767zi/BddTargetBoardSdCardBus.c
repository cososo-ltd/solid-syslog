/* The NUCLEO-F767ZI's MicroSD card bus behind the board seam - see
 * BddTargetBoardSdCardBus.h. The card is on SPI4, over ST's HAL, on the CN9 Zio
 * connector; README.md beside this file holds the pin plan.
 *
 * A W25Q128 flash shares the bus, with a select of its own. Nothing here
 * talks to it, and its select is held high so that it stays off the bus. */

#include "BddTargetBoardSdCardBus.h"

#include "Board.h"

#include "stm32f7xx_hal.h"

#include <stdbool.h>

/* All on port E. */
enum
{
    PIN_SCK = GPIO_PIN_2,
    PIN_FLASH_SELECT = GPIO_PIN_3,
    PIN_CARD_SELECT = GPIO_PIN_4,
    PIN_MISO = GPIO_PIN_5,
    PIN_MOSI = GPIO_PIN_6
};

enum
{
    /* SPI4 is clocked from APB2, which Board_Init runs at 54 MHz. Divided by
     * 256 that is 211 kHz, inside the 400 kHz a card may be started at. */
    START_RATE_PRESCALER = SPI_BAUDRATEPRESCALER_256,
    /* Divided by 16 it is 3.375 MHz. README.md says why no faster. */
    RUN_RATE_PRESCALER = SPI_BAUDRATEPRESCALER_16,

    /* A byte takes 38 microseconds at the start rate. */
    EXCHANGE_TIMEOUT_MILLISECONDS = 10,
    IDLE_LINE = 0xFF,
    CRC_POLYNOMIAL_UNUSED = 7
};

static SPI_HandleTypeDef spi;
static bool taken;

static inline void BddTargetBoardSdCardBus_TakePins(void);
static inline void BddTargetBoardSdCardBus_SetRate(uint32_t prescaler);

void BddTargetBoardSdCardBus_UseStartRate(void)
{
    if (!taken)
    {
        BddTargetBoardSdCardBus_TakePins();
        taken = true;
    }
    BddTargetBoardSdCardBus_SetRate(START_RATE_PRESCALER);
}

static inline void BddTargetBoardSdCardBus_TakePins(void)
{
    GPIO_InitTypeDef pins = {0};
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_SPI4_CLK_ENABLE();

    /* The levels are set before the pins are driven, so neither select is
     * seen low as the pins become outputs. */
    HAL_GPIO_WritePin(GPIOE, PIN_CARD_SELECT | PIN_FLASH_SELECT, GPIO_PIN_SET);
    pins.Pin = PIN_CARD_SELECT | PIN_FLASH_SELECT;
    pins.Mode = GPIO_MODE_OUTPUT_PP;
    pins.Pull = GPIO_NOPULL;
    pins.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &pins);

    pins.Pin = PIN_SCK | PIN_MOSI;
    pins.Mode = GPIO_MODE_AF_PP;
    pins.Speed = GPIO_SPEED_FREQ_MEDIUM;
    pins.Alternate = GPIO_AF5_SPI4;
    HAL_GPIO_Init(GPIOE, &pins);

    /* Pulled up, so a bus with no card on it reads as all ones. */
    pins.Pin = PIN_MISO;
    pins.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &pins);
}

static inline void BddTargetBoardSdCardBus_SetRate(uint32_t prescaler)
{
    spi.Instance = SPI4;
    spi.Init.Mode = SPI_MODE_MASTER;
    spi.Init.Direction = SPI_DIRECTION_2LINES;
    spi.Init.DataSize = SPI_DATASIZE_8BIT;
    spi.Init.CLKPolarity = SPI_POLARITY_LOW;
    spi.Init.CLKPhase = SPI_PHASE_1EDGE;
    spi.Init.NSS = SPI_NSS_SOFT;
    spi.Init.BaudRatePrescaler = prescaler;
    spi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi.Init.TIMode = SPI_TIMODE_DISABLE;
    spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    spi.Init.CRCPolynomial = CRC_POLYNOMIAL_UNUSED;
    spi.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    spi.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    /* A bus that will not configure exchanges nothing, which the card driver
     * reads as a card that does not answer. */
    (void) HAL_SPI_Init(&spi);
}

void BddTargetBoardSdCardBus_UseRunRate(void)
{
    BddTargetBoardSdCardBus_SetRate(RUN_RATE_PRESCALER);
}

void BddTargetBoardSdCardBus_Select(void)
{
    HAL_GPIO_WritePin(GPIOE, PIN_CARD_SELECT, GPIO_PIN_RESET);
}

void BddTargetBoardSdCardBus_Deselect(void)
{
    HAL_GPIO_WritePin(GPIOE, PIN_CARD_SELECT, GPIO_PIN_SET);
}

uint8_t BddTargetBoardSdCardBus_Exchange(uint8_t out)
{
    uint8_t in = IDLE_LINE;
    if (HAL_SPI_TransmitReceive(&spi, &out, &in, 1U, EXCHANGE_TIMEOUT_MILLISECONDS) != HAL_OK)
    {
        in = IDLE_LINE;
    }
    return in;
}

uint32_t BddTargetBoardSdCardBus_Milliseconds(void)
{
    return Board_Milliseconds();
}
