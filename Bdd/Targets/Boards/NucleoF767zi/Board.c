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

    /* Above the millisecond tick, which the HAL leaves at the lowest
     * priority, 15. The handler calls nothing in a kernel, so where this sits
     * against a kernel's own ceiling does not matter. */
    CONSOLE_INTERRUPT_PRIORITY = 6,

    /* The millisecond count comes from TIM6, a basic timer nothing else on the
     * board wants, counting microseconds up to one millisecond. SysTick is
     * left alone for a kernel to take. */
    TICK_TIMER_HZ = 1000000,
    TICK_TIMER_PERIOD = 1000,

    /* 8 MHz in, divided to 2 MHz, multiplied to a 432 MHz VCO, halved to a
     * 216 MHz system clock. Q gives the 48 MHz the USB, RNG and SDMMC want. */
    /* A character leaves in under 100 microseconds, a few thousand turns of
     * the loop that waits for it; this is long enough to be sure it has not. */
    FAULT_WAIT_LOOPS = 1000000,

    /* Where the core's stacked registers hold the two a fault report gives,
     * and where each figure goes in the report's text. */
    STACKED_LR = 5,
    STACKED_PC = 6,
    FAULT_PC_AT = 5,
    FAULT_LR_AT = 19,
    FAULT_CFSR_AT = 35,
    HEX_DIGITS = 8,
    BITS_PER_HEX_DIGIT = 4,

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
static inline uint32_t Board_TickTimerClockHz(void);
static inline void Board_ConfigureConsole(void);
static inline uint16_t Board_LedPin(enum BoardLed led);
static inline void Board_ConsoleRingPut(uint8_t received);
static void Board_FaultWrite(const char* text);
static inline void Board_FaultWaitFor(uint32_t flag);
static inline void Board_PutHex(char* eightDigits, uint32_t value);

/* Reached from HardFault_Handler's assembler, and by nothing else. */
void Board_ReportHardFault(const uint32_t* stacked);
/* The name ST's vector table carries. */
void HardFault_Handler(void);

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
        Board_Halt(true);
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
        Board_Halt(true);
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

void Board_Halt(bool withRedLed)
{
    __disable_irq();
    if (withRedLed)
    {
        HAL_GPIO_WritePin(GPIOB, LED_RED_PIN, GPIO_PIN_SET);
    }
    for (;;)
    {
    }
}

void Board_Fault(const char* what, const char* where)
{
    __disable_irq();
    Board_FaultWrite("\r\n[board] fault: ");
    Board_FaultWrite(what);
    Board_FaultWrite(" ");
    Board_FaultWrite(where);
    Board_FaultWrite("\r\n");
    Board_Halt(true);
}

/* The HAL's transmit is not used: the fault may have interrupted one, and
 * the HAL would refuse a second. */
static void Board_FaultWrite(const char* text)
{
    for (const char* next = text; *next != '\0'; next++)
    {
        Board_FaultWaitFor(USART_ISR_TXE);
        USART3->TDR = (uint8_t) *next;
    }
    Board_FaultWaitFor(USART_ISR_TC);
}

/* Bounded, because a fault before the console was opened finds a transmitter
 * that never empties, and the halt and its LED still have to be reached. */
static inline void Board_FaultWaitFor(uint32_t flag)
{
    for (uint32_t i = 0U; ((USART3->ISR & flag) == 0U) && (i < (uint32_t) FAULT_WAIT_LOOPS); i++)
    {
    }
}

/* Called by HardFault_Handler with the registers the core stacked as it took
 * the fault, of which the sixth and seventh are the link register and the
 * address of the instruction that faulted. */
void Board_ReportHardFault(const uint32_t* stacked)
{
    char where[] = "pc=0x00000000 lr=0x00000000 cfsr=0x00000000";
    Board_PutHex(&where[FAULT_PC_AT], stacked[STACKED_PC]);
    Board_PutHex(&where[FAULT_LR_AT], stacked[STACKED_LR]);
    Board_PutHex(&where[FAULT_CFSR_AT], SCB->CFSR);
    Board_Fault("hard fault", where);
}

static inline void Board_PutHex(char* eightDigits, uint32_t value)
{
    static const char DIGITS[] = "0123456789abcdef";
    for (uint32_t i = 0U; i < (uint32_t) HEX_DIGITS; i++)
    {
        uint32_t shift = ((uint32_t) HEX_DIGITS - 1U - i) * (uint32_t) BITS_PER_HEX_DIGIT;
        eightDigits[i] = DIGITS[(value >> shift) & 0xFU];
    }
}

/* The core stacks the registers on whichever stack was in use, which bit 2 of
 * the link register names. No fault but this one is enabled, so a memory,
 * bus or usage fault arrives here as well. The name is the one ST's vector
 * table carries. */
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile("tst lr, #4\n"
                   "ite eq\n"
                   "mrseq r0, msp\n"
                   "mrsne r0, psp\n"
                   "b Board_ReportHardFault\n");
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

/* The HAL calls this from HAL_Init, and again whenever the clock tree
 * changes, to start its millisecond count. ST's own version, which this
 * replaces, takes SysTick. The parameter is named as ST's prototype has it. */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) // NOLINT(readability-identifier-naming) -- ST's name
{
    __HAL_RCC_TIM6_CLK_ENABLE();
    TIM6->CR1 = 0U;
    TIM6->PSC = (Board_TickTimerClockHz() / (uint32_t) TICK_TIMER_HZ) - 1U;
    TIM6->ARR = (uint32_t) TICK_TIMER_PERIOD - 1U;
    /* Load the prescaler now, not at the first overflow, then drop the update
     * that doing so raises. */
    TIM6->EGR = TIM_EGR_UG;
    TIM6->SR = 0U;
    TIM6->DIER = TIM_DIER_UIE;
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, TickPriority, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    TIM6->CR1 = TIM_CR1_CEN;
    uwTickPrio = TickPriority;
    return HAL_OK;
}

/* A timer on APB1 is clocked at the bus frequency, or at twice it when the
 * bus is divided down from the core clock. */
static inline uint32_t Board_TickTimerClockHz(void)
{
    RCC_ClkInitTypeDef clocks = {0};
    uint32_t flashLatency = 0U;
    HAL_RCC_GetClockConfig(&clocks, &flashLatency);
    uint32_t timerClock = HAL_RCC_GetPCLK1Freq();
    if (clocks.APB1CLKDivider != RCC_HCLK_DIV1)
    {
        timerClock *= 2U;
    }
    return timerClock;
}

/* The millisecond tick. The name is the one ST's vector table carries. */
void TIM6_DAC_IRQHandler(void)
{
    TIM6->SR = 0U;
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
