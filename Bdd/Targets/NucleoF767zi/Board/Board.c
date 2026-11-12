/* Board support for the NUCLEO-F767ZI over ST's HAL.
 *
 * Not host-TDD'd: this file exists only in the cross build, and what it does
 * is write the part's registers. The target checklist in README.md is what
 * proves it, on the board. */

#include "Board.h"

#include "stm32f7xx_hal.h"

enum
{
    /* The user LEDs, all on port B. */
    LED_GREEN_PIN = GPIO_PIN_0,
    LED_BLUE_PIN = GPIO_PIN_7,
    LED_RED_PIN = GPIO_PIN_14,

    /* USART3, which the ST-LINK carries as its virtual COM port. Port D. */
    CONSOLE_TX_PIN = GPIO_PIN_8,
    CONSOLE_RX_PIN = GPIO_PIN_9,
    CONSOLE_BAUD_RATE = 115200,

    /* HAL_UART_Transmit takes its length as 16 bits. */
    CONSOLE_WRITE_CHUNK = 0xFFFF,

    /* 8 MHz in, divided to 2 MHz, multiplied to a 432 MHz VCO, halved to a
     * 216 MHz system clock. Q gives the 48 MHz the USB, RNG and SDMMC want. */
    PLL_M = 4,
    PLL_N = 216,
    PLL_Q = 9
};

static UART_HandleTypeDef console;

static inline void Board_ConfigureLeds(void);
static inline void Board_ConfigureClock(void);
static inline bool Board_StartPll(void);
static inline bool Board_SwitchToPll(void);
static inline void Board_HaltWithRedLed(void);
static inline void Board_ConfigureConsole(void);
static inline uint16_t Board_LedPin(enum BoardLed led);

void Board_Init(void)
{
    HAL_Init();
    Board_ConfigureLeds();
    Board_ConfigureClock();
    Board_ConfigureConsole();
}

static inline void Board_ConfigureLeds(void)
{
    GPIO_InitTypeDef leds = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    leds.Pin = LED_GREEN_PIN | LED_BLUE_PIN | LED_RED_PIN;
    leds.Mode = GPIO_MODE_OUTPUT_PP;
    leds.Pull = GPIO_NOPULL;
    leds.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &leds);
}

static inline void Board_ConfigureClock(void)
{
    bool started = Board_StartPll();
    if (started)
    {
        started = Board_SwitchToPll();
    }
    if (!started)
    {
        Board_HaltWithRedLed();
    }
}

static inline bool Board_StartPll(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscillator.HSEState = RCC_HSE_BYPASS;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscillator.PLL.PLLM = PLL_M;
    oscillator.PLL.PLLN = PLL_N;
    oscillator.PLL.PLLP = RCC_PLLP_DIV2;
    oscillator.PLL.PLLQ = PLL_Q;
    oscillator.PLL.PLLR = 2;
    /* Above 180 MHz the regulator has to be in over-drive before the switch. */
    return (HAL_RCC_OscConfig(&oscillator) == HAL_OK) && (HAL_PWREx_EnableOverDrive() == HAL_OK);
}

static inline bool Board_SwitchToPll(void)
{
    RCC_ClkInitTypeDef clocks = {0};
    clocks.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
    /* APB1 is limited to 54 MHz and APB2 to 108 MHz. */
    clocks.APB1CLKDivider = RCC_HCLK_DIV4;
    clocks.APB2CLKDivider = RCC_HCLK_DIV2;
    return HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_7) == HAL_OK;
}

static inline void Board_HaltWithRedLed(void)
{
    HAL_GPIO_WritePin(GPIOB, LED_RED_PIN, GPIO_PIN_SET);
    for (;;)
    {
    }
}

static inline void Board_ConfigureConsole(void)
{
    GPIO_InitTypeDef pins = {0};
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    pins.Pin = CONSOLE_TX_PIN | CONSOLE_RX_PIN;
    pins.Mode = GPIO_MODE_AF_PP;
    pins.Pull = GPIO_PULLUP;
    pins.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    pins.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &pins);

    console.Instance = USART3;
    console.Init.BaudRate = CONSOLE_BAUD_RATE;
    console.Init.WordLength = UART_WORDLENGTH_8B;
    console.Init.StopBits = UART_STOPBITS_1;
    console.Init.Parity = UART_PARITY_NONE;
    console.Init.Mode = UART_MODE_TX_RX;
    console.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    console.Init.OverSampling = UART_OVERSAMPLING_16;
    console.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    /* The console is polled, so a byte can arrive before the last was read.
     * With overrun detection on, that would stop the receiver until the flag
     * was cleared; off, the newer byte simply replaces the older. */
    console.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT;
    console.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
    if (HAL_UART_Init(&console) != HAL_OK)
    {
        Board_HaltWithRedLed();
    }
}

void Board_ConsoleWrite(const char* text, size_t length)
{
    while (length > 0U)
    {
        size_t chunk = (length > (size_t) CONSOLE_WRITE_CHUNK) ? (size_t) CONSOLE_WRITE_CHUNK : length;
        (void) HAL_UART_Transmit(&console, (const uint8_t*) text, (uint16_t) chunk, HAL_MAX_DELAY);
        text += chunk;
        length -= chunk;
    }
}

bool Board_ConsoleTryRead(char* received)
{
    bool arrived = __HAL_UART_GET_FLAG(&console, UART_FLAG_RXNE) != 0U;
    if (arrived)
    {
        *received = (char) (console.Instance->RDR & 0xFFU);
    }
    return arrived;
}

uint32_t Board_Milliseconds(void)
{
    return HAL_GetTick();
}

void Board_LedToggle(enum BoardLed led)
{
    HAL_GPIO_TogglePin(GPIOB, Board_LedPin(led));
}

static inline uint16_t Board_LedPin(enum BoardLed led)
{
    uint16_t pin = LED_GREEN_PIN;
    if (led == BOARD_LED_BLUE)
    {
        pin = LED_BLUE_PIN;
    }
    else if (led == BOARD_LED_RED)
    {
        pin = LED_RED_PIN;
    }
    else
    {
        /* Green, already chosen. */
    }
    return pin;
}

/* The HAL's millisecond count is driven from the SysTick interrupt. The name
 * is the one ST's vector table carries. */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
