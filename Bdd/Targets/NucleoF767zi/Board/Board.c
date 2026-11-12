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

    /* Received bytes wait here between the interrupt and the reader. A power
     * of two, so an index wraps with a mask; one slot always stays empty. */
    CONSOLE_RING_SIZE = 512,
    CONSOLE_RING_MASK = CONSOLE_RING_SIZE - 1,

    /* Above SysTick, which the HAL leaves at the lowest priority, 15. */
    CONSOLE_INTERRUPT_PRIORITY = 6,

    /* 8 MHz in, divided to 2 MHz, multiplied to a 432 MHz VCO, halved to a
     * 216 MHz system clock. Q gives the 48 MHz the USB, RNG and SDMMC want. */
    PLL_M = 4,
    PLL_N = 216,
    PLL_Q = 9
};

static UART_HandleTypeDef console;

/* Written by the receive interrupt and read by Board_ConsoleTryRead. The
 * interrupt alone moves the head and the reader alone moves the tail, and
 * each is one aligned word, so neither side needs the other held off. */
static volatile uint8_t consoleRing[CONSOLE_RING_SIZE];
static volatile uint32_t consoleRingHead;
static volatile uint32_t consoleRingTail;

static inline void Board_ConfigureLeds(void);
static inline void Board_ConfigureClock(void);
static inline bool Board_StartPll(void);
static inline bool Board_SwitchToPll(void);
static inline void Board_HaltWithRedLed(void);
static inline void Board_ConfigureConsole(void);
static inline uint16_t Board_LedPin(enum BoardLed led);
static inline void Board_ConsoleRingPut(uint8_t received);

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
    /* With overrun detection on, a byte arriving before the interrupt had
     * taken the last would stop the receiver until the flag was cleared; off,
     * the newer byte simply replaces the older. */
    console.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT;
    console.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
    if (HAL_UART_Init(&console) != HAL_OK)
    {
        Board_HaltWithRedLed();
    }
    __HAL_UART_ENABLE_IT(&console, UART_IT_RXNE);
    HAL_NVIC_SetPriority(USART3_IRQn, CONSOLE_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
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
    uint32_t tail = consoleRingTail;
    bool arrived = tail != consoleRingHead;
    if (arrived)
    {
        *received = (char) consoleRing[tail];
        consoleRingTail = (tail + 1U) & (uint32_t) CONSOLE_RING_MASK;
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

/* The console's receive interrupt. Reading the data register clears the
 * request. The name is the one ST's vector table carries. */
void USART3_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&console, UART_FLAG_RXNE) != 0U)
    {
        Board_ConsoleRingPut((uint8_t) (console.Instance->RDR & 0xFFU));
    }
}

/* A byte that finds the ring full is dropped. */
static inline void Board_ConsoleRingPut(uint8_t received)
{
    uint32_t head = consoleRingHead;
    uint32_t next = (head + 1U) & (uint32_t) CONSOLE_RING_MASK;
    if (next != consoleRingTail)
    {
        consoleRing[head] = received;
        consoleRingHead = next;
    }
}
