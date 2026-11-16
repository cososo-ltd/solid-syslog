/* The NUCLEO-F767ZI's SPI4, over ST's HAL - see BoardSpi4.h. */

#include "BoardSpi4.h"

#include "stm32f7xx_hal.h"

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
     * 256 that is 211 kHz, and divided by 8 it is 6.75 MHz. */
    START_RATE_PRESCALER = SPI_BAUDRATEPRESCALER_256,
    RUN_RATE_PRESCALER = SPI_BAUDRATEPRESCALER_8,

    /* A byte takes 38 microseconds at the start rate, a few thousand turns of
     * the loop that waits for it. This is many times that. */
    EXCHANGE_SPINS = 1000000,
    IDLE_LINE = 0xFF,
    CRC_POLYNOMIAL_UNUSED = 7
};

static SPI_HandleTypeDef spi;

static inline void BoardSpi4_TakePins(void);
static inline uint16_t BoardSpi4_SelectPin(enum BoardSpi4Device device);

void BoardSpi4_Take(enum BoardSpi4Rate rate)
{
    BoardSpi4_TakePins();
    BoardSpi4_SetRate(rate);
}

static inline void BoardSpi4_TakePins(void)
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

    /* Pulled up, so a bus with nothing selected on it reads as all ones. */
    pins.Pin = PIN_MISO;
    pins.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &pins);
}

void BoardSpi4_SetRate(enum BoardSpi4Rate rate)
{
    spi.Instance = SPI4;
    spi.Init.Mode = SPI_MODE_MASTER;
    spi.Init.Direction = SPI_DIRECTION_2LINES;
    spi.Init.DataSize = SPI_DATASIZE_8BIT;
    spi.Init.CLKPolarity = SPI_POLARITY_LOW;
    spi.Init.CLKPhase = SPI_PHASE_1EDGE;
    spi.Init.NSS = SPI_NSS_SOFT;
    spi.Init.BaudRatePrescaler =
        (rate == BOARD_SPI4_RATE_START) ? (uint32_t) START_RATE_PRESCALER : (uint32_t) RUN_RATE_PRESCALER;
    spi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi.Init.TIMode = SPI_TIMODE_DISABLE;
    spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    spi.Init.CRCPolynomial = CRC_POLYNOMIAL_UNUSED;
    spi.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    spi.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    /* A bus that will not configure exchanges nothing, which a driver sees as
     * a device that does not answer. HAL_SPI_Init leaves the peripheral
     * disabled, with RXNE set by one received byte and not two. */
    if (HAL_SPI_Init(&spi) == HAL_OK)
    {
        __HAL_SPI_ENABLE(&spi);
    }
}

void BoardSpi4_Select(enum BoardSpi4Device device)
{
    HAL_GPIO_WritePin(GPIOE, BoardSpi4_SelectPin(device), GPIO_PIN_RESET);
}

static inline uint16_t BoardSpi4_SelectPin(enum BoardSpi4Device device)
{
    return (device == BOARD_SPI4_CARD) ? (uint16_t) PIN_CARD_SELECT : (uint16_t) PIN_FLASH_SELECT;
}

void BoardSpi4_Deselect(enum BoardSpi4Device device)
{
    HAL_GPIO_WritePin(GPIOE, BoardSpi4_SelectPin(device), GPIO_PIN_SET);
}

/* The byte is exchanged through the data register directly, not through the
 * HAL's transfer call. That call times each transfer against the millisecond
 * count, so a task switched out for longer than the timeout is told that a
 * byte which was exchanged had failed. Here the byte that arrived is still in
 * the register when the task returns. The wait is bounded by a count, so a
 * peripheral that never delivers reads as an idle line.
 *
 * Both accesses are a byte wide, so that one byte goes into the transmit FIFO
 * and one comes out of the receive FIFO. */
uint8_t BoardSpi4_Exchange(uint8_t out)
{
    uint8_t in = IDLE_LINE;
    uint32_t spins = EXCHANGE_SPINS;
    *(volatile uint8_t*) &SPI4->DR = out;
    while (((SPI4->SR & SPI_SR_RXNE) == 0U) && (spins > 0U))
    {
        spins--;
    }
    if ((SPI4->SR & SPI_SR_RXNE) != 0U)
    {
        in = *(volatile uint8_t*) &SPI4->DR;
    }
    return in;
}
