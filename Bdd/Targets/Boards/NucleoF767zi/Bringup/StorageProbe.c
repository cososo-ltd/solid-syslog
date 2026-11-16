/* The storage probe of the bring-up image - see StorageProbe.h. */

#include "StorageProbe.h"

#include "Board.h"

#include "stm32f7xx_hal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* SPI4 on the CN9 Zio connector, all on port E. */
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
    /* Half a clock period, as turns of an empty loop. It puts the clock in
     * the low hundreds of kHz, within the 100 to 400 kHz for starting a card;
     * the probe does not need an exact rate. */
    HALF_PERIOD_LOOPS = 150,

    IDLE_BYTE = 0xFF,

    /* A card needs 74 clocks with its select high before its first command. */
    CARD_WAKE_BYTES = 10,
    /* A card answers a command within eight bytes. */
    CARD_RESPONSE_WAIT_BYTES = 8,
    CARD_GO_IDLE_ATTEMPTS = 10,
    CARD_COMMAND_LENGTH = 6,
    CARD_INTERFACE_CONDITION_LENGTH = 4,
    CARD_IN_IDLE_STATE = 0x01,

    FLASH_READ_JEDEC_ID = 0x9F,
    FLASH_JEDEC_ID_LENGTH = 3,

    HEX_DIGITS_PER_BYTE = 2,
    TEXT_LENGTH = 96
};

/* CMD0, GO_IDLE_STATE, and CMD8, SEND_IF_COND for 2.7 to 3.6 V with the check
 * pattern 0xAA, each with its CRC: a card checks it until it is in SPI mode. */
static const uint8_t CARD_GO_IDLE[CARD_COMMAND_LENGTH] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
static const uint8_t CARD_SEND_INTERFACE_CONDITION[CARD_COMMAND_LENGTH] = {0x48, 0x00, 0x00, 0x01, 0xAA, 0x87};

struct ProbeText
{
    char Text[TEXT_LENGTH];
    size_t Length;
};

static inline void StorageProbe_ProbeCard(void);
static inline void StorageProbe_WakeCard(void);
static inline uint8_t StorageProbe_CardGoIdle(void);
static uint8_t StorageProbe_CardCommand(const uint8_t* command);
static inline void StorageProbe_ProbeFlash(void);
static inline void StorageProbe_Select(uint16_t pin);
static inline void StorageProbe_Deselect(uint16_t pin);
static uint8_t StorageProbe_Transfer(uint8_t out);
static inline void StorageProbe_HalfPeriod(void);
static void ProbeText_Append(struct ProbeText* text, const char* more);
static void ProbeText_AppendHex(struct ProbeText* text, const uint8_t* bytes, size_t count);
static inline void ProbeText_Write(const struct ProbeText* text);

void StorageProbe_Init(void)
{
    GPIO_InitTypeDef pins = {0};
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* The levels are set before the pins are driven, so neither select is
     * seen low as the pins become outputs. */
    HAL_GPIO_WritePin(GPIOE, PIN_CARD_SELECT | PIN_FLASH_SELECT | PIN_MOSI, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOE, PIN_SCK, GPIO_PIN_RESET);

    pins.Pin = PIN_SCK | PIN_MOSI | PIN_CARD_SELECT | PIN_FLASH_SELECT;
    pins.Mode = GPIO_MODE_OUTPUT_PP;
    pins.Pull = GPIO_NOPULL;
    pins.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &pins);

    /* Pulled up, so a device that is not there reads as all ones. */
    pins.Pin = PIN_MISO;
    pins.Mode = GPIO_MODE_INPUT;
    pins.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &pins);
}

void StorageProbe_Run(void)
{
    StorageProbe_Init();
    StorageProbe_ProbeCard();
    StorageProbe_ProbeFlash();
}

static inline void StorageProbe_ProbeCard(void)
{
    struct ProbeText text = {{0}, 0};
    uint8_t interfaceCondition[CARD_INTERFACE_CONDITION_LENGTH] = {0};

    StorageProbe_WakeCard();
    uint8_t idle = StorageProbe_CardGoIdle();

    StorageProbe_Select(PIN_CARD_SELECT);
    uint8_t condition = StorageProbe_CardCommand(CARD_SEND_INTERFACE_CONDITION);
    for (size_t i = 0; i < (size_t) CARD_INTERFACE_CONDITION_LENGTH; i++)
    {
        interfaceCondition[i] = StorageProbe_Transfer(IDLE_BYTE);
    }
    StorageProbe_Deselect(PIN_CARD_SELECT);

    ProbeText_Append(&text, "probe: card CMD0 ");
    ProbeText_AppendHex(&text, &idle, 1);
    ProbeText_Append(&text, " CMD8 ");
    ProbeText_AppendHex(&text, &condition, 1);
    ProbeText_Append(&text, " ");
    ProbeText_AppendHex(&text, interfaceCondition, CARD_INTERFACE_CONDITION_LENGTH);
    ProbeText_Append(&text, (idle == (uint8_t) CARD_IN_IDLE_STATE) ? " - answered\r\n" : " - no card answered\r\n");
    ProbeText_Write(&text);
}

static inline void StorageProbe_WakeCard(void)
{
    for (size_t i = 0; i < (size_t) CARD_WAKE_BYTES; i++)
    {
        (void) StorageProbe_Transfer(IDLE_BYTE);
    }
}

static uint8_t StorageProbe_Transfer(uint8_t out)
{
    /* SPI mode 0, most significant bit first: the line is set while the
     * clock is low and both ends take it on the rising edge. */
    uint8_t in = 0;
    for (uint8_t mask = 0x80; mask != 0U; mask >>= 1U)
    {
        HAL_GPIO_WritePin(GPIOE, PIN_MOSI, ((out & mask) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        StorageProbe_HalfPeriod();
        HAL_GPIO_WritePin(GPIOE, PIN_SCK, GPIO_PIN_SET);
        if (HAL_GPIO_ReadPin(GPIOE, PIN_MISO) == GPIO_PIN_SET)
        {
            in |= mask;
        }
        StorageProbe_HalfPeriod();
        HAL_GPIO_WritePin(GPIOE, PIN_SCK, GPIO_PIN_RESET);
    }
    return in;
}

static inline void StorageProbe_HalfPeriod(void)
{
    for (volatile uint32_t loops = HALF_PERIOD_LOOPS; loops != 0U; loops--)
    {
    }
}

/* A card that has just been powered may not answer its first CMD0. */
static inline uint8_t StorageProbe_CardGoIdle(void)
{
    uint8_t response = IDLE_BYTE;
    for (size_t attempt = 0; (attempt < (size_t) CARD_GO_IDLE_ATTEMPTS) && (response != (uint8_t) CARD_IN_IDLE_STATE);
         attempt++)
    {
        StorageProbe_Select(PIN_CARD_SELECT);
        response = StorageProbe_CardCommand(CARD_GO_IDLE);
        StorageProbe_Deselect(PIN_CARD_SELECT);
    }
    return response;
}

static inline void StorageProbe_Select(uint16_t pin)
{
    HAL_GPIO_WritePin(GPIOE, pin, GPIO_PIN_RESET);
}

/* Sends the command and returns the first byte of its answer, which is the
 * first byte with its top bit clear, or IDLE_BYTE if none came. */
static uint8_t StorageProbe_CardCommand(const uint8_t* command)
{
    uint8_t response = IDLE_BYTE;
    for (size_t i = 0; i < (size_t) CARD_COMMAND_LENGTH; i++)
    {
        (void) StorageProbe_Transfer(command[i]);
    }
    for (size_t waited = 0; (waited < (size_t) CARD_RESPONSE_WAIT_BYTES) && ((response & 0x80U) != 0U); waited++)
    {
        response = StorageProbe_Transfer(IDLE_BYTE);
    }
    return response;
}

/* A card releases its data line only after eight more clocks. */
static inline void StorageProbe_Deselect(uint16_t pin)
{
    HAL_GPIO_WritePin(GPIOE, pin, GPIO_PIN_SET);
    (void) StorageProbe_Transfer(IDLE_BYTE);
}

static void ProbeText_Append(struct ProbeText* text, const char* more)
{
    size_t length = strlen(more);
    if (length <= ((size_t) TEXT_LENGTH - text->Length))
    {
        (void) memcpy(&text->Text[text->Length], more, length);
        text->Length += length;
    }
}

static void ProbeText_AppendHex(struct ProbeText* text, const uint8_t* bytes, size_t count)
{
    static const char DIGITS[] = "0123456789ABCDEF";
    for (size_t i = 0; (i < count) && (((size_t) TEXT_LENGTH - text->Length) >= (size_t) HEX_DIGITS_PER_BYTE); i++)
    {
        text->Text[text->Length] = DIGITS[bytes[i] >> 4U];
        text->Text[text->Length + 1U] = DIGITS[bytes[i] & 0x0FU];
        text->Length += (size_t) HEX_DIGITS_PER_BYTE;
    }
}

static inline void ProbeText_Write(const struct ProbeText* text)
{
    Board_ConsoleWrite(text->Text, text->Length);
}

static inline void StorageProbe_ProbeFlash(void)
{
    struct ProbeText text = {{0}, 0};
    uint8_t id[FLASH_JEDEC_ID_LENGTH] = {0};

    StorageProbe_Select(PIN_FLASH_SELECT);
    (void) StorageProbe_Transfer(FLASH_READ_JEDEC_ID);
    for (size_t i = 0; i < (size_t) FLASH_JEDEC_ID_LENGTH; i++)
    {
        id[i] = StorageProbe_Transfer(IDLE_BYTE);
    }
    StorageProbe_Deselect(PIN_FLASH_SELECT);

    bool answered = (id[0] != (uint8_t) IDLE_BYTE) && (id[0] != 0U);
    ProbeText_Append(&text, "probe: flash JEDEC id ");
    ProbeText_AppendHex(&text, id, FLASH_JEDEC_ID_LENGTH);
    ProbeText_Append(&text, answered ? " - answered\r\n" : " - no flash answered\r\n");
    ProbeText_Write(&text);
}
