/* An SD card in SPI mode - see BddTargetSdCard.h.
 *
 * The section numbers are those of the SD Physical Layer Simplified
 * Specification. */

#include "BddTargetSdCard.h"

#include "BddTargetBoardSdCardBus.h"

#include <stddef.h>

enum
{
    /* A line with nothing driving it, and what is clocked out to read. */
    IDLE_LINE = 0xFF,

    /* Section 6.4.1.1: 74 clocks with the select high before the first command. */
    WAKE_BYTES = 10,
    /* A card just powered may let its first commands pass. */
    GO_IDLE_ATTEMPTS = 10,

    /* Section 7.3.1.1: a start bit, a transmission bit, six bits of index,
     * four bytes of argument, seven bits of CRC and an end bit. */
    COMMAND_LENGTH = 6,
    COMMAND_START = 0x40,
    COMMAND_END_BIT = 0x01,
    /* Section 7.3.2.1: the answer comes within eight bytes, its top bit clear. */
    ANSWER_WAIT_BYTES = 8,
    ANSWER_NOT_YET = 0x80,
    ANSWER_READY = 0x00,
    ANSWER_IDLE = 0x01,
    ANSWER_ILLEGAL_COMMAND = 0x04,
    ANSWER_CRC_ERROR = 0x08,

    GO_IDLE_STATE = 0,
    SEND_IF_COND = 8,
    SEND_CSD = 9,
    SEND_STATUS = 13,
    SET_BLOCKLEN = 16,
    READ_SINGLE_BLOCK = 17,
    WRITE_BLOCK = 24,
    SD_SEND_OP_COND = 41,
    APP_CMD = 55,
    READ_OCR = 58,
    CRC_ON_OFF = 59,

    CRC_ON = 1,
    /* Section 4.3.13: 2.7 to 3.6 V, and a pattern the card is to send back. */
    INTERFACE_CONDITION_LENGTH = 4,
    VOLTAGE_2V7_TO_3V6 = 0x01,
    CHECK_PATTERN = 0xAA,
    VOLTAGE_ACCEPTED_MASK = 0x0F,
    /* Section 5.1: the card capacity status bit, in the first byte of the OCR. */
    OCR_LENGTH = 4,
    OCR_HIGH_CAPACITY = 0x40,
    CSD_LENGTH = 16,

    /* Section 7.3.3: the token before a block, and the card's word on a block
     * it was sent. */
    DATA_TOKEN = 0xFE,
    DATA_RESPONSE_MASK = 0x1F,
    DATA_ACCEPTED = 0x05,
    DATA_CRC_ERROR = 0x0B,
    CRC_LENGTH = 2,

    /* Section 4.6.2: a card is ready within a second of being told to start,
     * sends what it was asked to read within 100 ms and finishes a write
     * within 250 ms. The last two are doubled here. */
    START_TIMEOUT_MILLISECONDS = 1000,
    READ_TIMEOUT_MILLISECONDS = 200,
    BUSY_TIMEOUT_MILLISECONDS = 500,

    /* A transfer that fails its CRC is tried this many times in all. */
    TRANSFER_ATTEMPTS = 3
};

/* Bit 30 of ACMD41's argument: the host takes high capacity cards. */
static const uint32_t HOST_HIGH_CAPACITY = 0x40000000U;

static bool started;
static bool blockAddressed;
static uint32_t sectorCount;
static uint32_t repeats;

static inline void SdCard_Wake(void);
static enum BddTargetSdCardResult SdCard_GoIdle(void);
static enum BddTargetSdCardResult SdCard_Command(uint8_t index, uint32_t argument, uint8_t* answer);
static enum BddTargetSdCardResult SdCard_Begin(uint8_t index, uint32_t argument, uint8_t* answer);
static bool SdCard_WaitUntilReady(void);
static inline bool SdCard_HasPassed(uint32_t since, uint32_t milliseconds);
static inline uint8_t SdCard_Crc7(const uint8_t* bytes, size_t count);
static inline void SdCard_End(void);
static inline enum BddTargetSdCardResult SdCard_Judge(uint8_t answer, uint8_t wanted);
static enum BddTargetSdCardResult SdCard_TurnCrcOn(void);
static enum BddTargetSdCardResult SdCard_CheckInterfaceCondition(void);
static inline void SdCard_Receive(uint8_t* bytes, size_t count);
static enum BddTargetSdCardResult SdCard_LeaveIdle(void);
static enum BddTargetSdCardResult SdCard_ReadAddressing(void);
static enum BddTargetSdCardResult SdCard_ReadCapacity(void);
static enum BddTargetSdCardResult SdCard_ReceiveBlock(uint8_t* bytes, size_t count);
static uint16_t SdCard_Crc16(const uint8_t* bytes, size_t count);
static inline uint32_t SdCard_SectorsInCsd(const uint8_t* csd);
static enum BddTargetSdCardResult SdCard_ReadOnce(uint32_t sector, uint8_t* buffer);
static inline uint32_t SdCard_AddressOf(uint32_t sector);
static enum BddTargetSdCardResult SdCard_WriteOnce(uint32_t sector, const uint8_t* buffer);
static enum BddTargetSdCardResult SdCard_SendBlock(const uint8_t* bytes, size_t count);
static enum BddTargetSdCardResult SdCard_CheckStatus(void);

enum BddTargetSdCardResult BddTargetSdCard_Start(void)
{
    BddTargetSdCard_Stop();
    BddTargetBoardSdCardBus_UseStartRate();
    SdCard_Wake();

    enum BddTargetSdCardResult result = SdCard_GoIdle();
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_TurnCrcOn();
    }
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_CheckInterfaceCondition();
    }
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_LeaveIdle();
    }
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_ReadAddressing();
    }
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_ReadCapacity();
    }
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        BddTargetBoardSdCardBus_UseRunRate();
        started = true;
    }
    return result;
}

static inline void SdCard_Wake(void)
{
    BddTargetBoardSdCardBus_Deselect();
    for (size_t i = 0; i < (size_t) WAKE_BYTES; i++)
    {
        (void) BddTargetBoardSdCardBus_Exchange(IDLE_LINE);
    }
}

static enum BddTargetSdCardResult SdCard_GoIdle(void)
{
    enum BddTargetSdCardResult result = BDD_TARGET_SD_CARD_NO_ANSWER;
    uint8_t answer = IDLE_LINE;
    for (size_t attempt = 0; (attempt < (size_t) GO_IDLE_ATTEMPTS) && (answer != (uint8_t) ANSWER_IDLE); attempt++)
    {
        result = SdCard_Command(GO_IDLE_STATE, 0U, &answer);
    }
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_Judge(answer, ANSWER_IDLE);
    }
    return result;
}

/* An answer that is not the one wanted is the card refusing, unless what it
 * says is that the command failed its CRC, which is the wires. */
static inline enum BddTargetSdCardResult SdCard_Judge(uint8_t answer, uint8_t wanted)
{
    enum BddTargetSdCardResult result = BDD_TARGET_SD_CARD_OK;
    if (answer != wanted)
    {
        result =
            ((answer & (uint8_t) ANSWER_CRC_ERROR) != 0U) ? BDD_TARGET_SD_CARD_CRC_FAILED : BDD_TARGET_SD_CARD_REFUSED;
    }
    return result;
}

/* A command whose answer is the one byte. */
static enum BddTargetSdCardResult SdCard_Command(uint8_t index, uint32_t argument, uint8_t* answer)
{
    enum BddTargetSdCardResult result = SdCard_Begin(index, argument, answer);
    SdCard_End();
    return result;
}

/* Selects the card, sends the command and reads the first byte of its answer.
 * The card is left selected, for the caller to read what follows, and every
 * Begin is matched by an End whatever it returns. */
static enum BddTargetSdCardResult SdCard_Begin(uint8_t index, uint32_t argument, uint8_t* answer)
{
    enum BddTargetSdCardResult result = BDD_TARGET_SD_CARD_OK;
    *answer = IDLE_LINE;
    BddTargetBoardSdCardBus_Select();
    if (!SdCard_WaitUntilReady())
    {
        result = BDD_TARGET_SD_CARD_TIMED_OUT;
    }
    else
    {
        uint8_t frame[COMMAND_LENGTH] = {
            (uint8_t) (COMMAND_START | index),
            (uint8_t) (argument >> 24U),
            (uint8_t) (argument >> 16U),
            (uint8_t) (argument >> 8U),
            (uint8_t) argument,
            0U,
        };
        frame[COMMAND_LENGTH - 1] =
            (uint8_t) ((uint8_t) (SdCard_Crc7(frame, (size_t) COMMAND_LENGTH - 1U) << 1U) | (uint8_t) COMMAND_END_BIT);
        for (size_t i = 0; i < (size_t) COMMAND_LENGTH; i++)
        {
            (void) BddTargetBoardSdCardBus_Exchange(frame[i]);
        }
        for (size_t waited = 0; (waited < (size_t) ANSWER_WAIT_BYTES) && ((*answer & (uint8_t) ANSWER_NOT_YET) != 0U);
             waited++)
        {
            *answer = BddTargetBoardSdCardBus_Exchange(IDLE_LINE);
        }
        if ((*answer & (uint8_t) ANSWER_NOT_YET) != 0U)
        {
            result = BDD_TARGET_SD_CARD_NO_ANSWER;
        }
    }
    return result;
}

/* A card holds its data line low while it is busy and lets it go when it is
 * not. A bus with no card on it reads as ready. */
static bool SdCard_WaitUntilReady(void)
{
    uint32_t since = BddTargetBoardSdCardBus_Milliseconds();
    bool ready = BddTargetBoardSdCardBus_Exchange(IDLE_LINE) == (uint8_t) IDLE_LINE;
    bool expired = false;
    while (!ready && !expired)
    {
        /* The time is read before the card is asked, so the card is always
         * asked once after the time allowed has passed. A task switched out
         * for longer than that would otherwise give up on a card that
         * finished while it was away. */
        expired = SdCard_HasPassed(since, BUSY_TIMEOUT_MILLISECONDS);
        ready = BddTargetBoardSdCardBus_Exchange(IDLE_LINE) == (uint8_t) IDLE_LINE;
    }
    return ready;
}

/* Unsigned, so it holds across the count wrapping. */
static inline bool SdCard_HasPassed(uint32_t since, uint32_t milliseconds)
{
    return (BddTargetBoardSdCardBus_Milliseconds() - since) >= milliseconds;
}

/* Section 4.5: x^7 + x^3 + 1 over the command's first five bytes. */
static inline uint8_t SdCard_Crc7(const uint8_t* bytes, size_t count)
{
    uint8_t crc = 0U;
    for (size_t i = 0; i < count; i++)
    {
        uint8_t byte = bytes[i];
        for (uint8_t bit = 0U; bit < 8U; bit++)
        {
            crc = (uint8_t) (crc << 1U);
            if (((byte ^ crc) & 0x80U) != 0U)
            {
                crc ^= 0x09U;
            }
            byte = (uint8_t) (byte << 1U);
        }
    }
    return (uint8_t) (crc & 0x7FU);
}

/* A card lets go of its data line only after eight more clocks. */
static inline void SdCard_End(void)
{
    BddTargetBoardSdCardBus_Deselect();
    (void) BddTargetBoardSdCardBus_Exchange(IDLE_LINE);
}

static enum BddTargetSdCardResult SdCard_TurnCrcOn(void)
{
    uint8_t answer = IDLE_LINE;
    enum BddTargetSdCardResult result = SdCard_Command(CRC_ON_OFF, CRC_ON, &answer);
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_Judge(answer, ANSWER_IDLE);
    }
    return result;
}

/* A card of the first version of the specification does not know this
 * command, and that is how it is told from one of the second. */
static enum BddTargetSdCardResult SdCard_CheckInterfaceCondition(void)
{
    uint8_t answer = IDLE_LINE;
    uint32_t argument = ((uint32_t) VOLTAGE_2V7_TO_3V6 << 8U) | (uint32_t) CHECK_PATTERN;
    enum BddTargetSdCardResult result = SdCard_Begin(SEND_IF_COND, argument, &answer);
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        if ((answer & (uint8_t) ANSWER_ILLEGAL_COMMAND) != 0U)
        {
            result = BDD_TARGET_SD_CARD_UNSUPPORTED;
        }
        else if (answer != (uint8_t) ANSWER_IDLE)
        {
            result = SdCard_Judge(answer, ANSWER_IDLE);
        }
        else
        {
            uint8_t condition[INTERFACE_CONDITION_LENGTH] = {0};
            SdCard_Receive(condition, INTERFACE_CONDITION_LENGTH);
            bool voltageAccepted = (condition[2] & (uint8_t) VOLTAGE_ACCEPTED_MASK) == (uint8_t) VOLTAGE_2V7_TO_3V6;
            if (!voltageAccepted || (condition[3] != (uint8_t) CHECK_PATTERN))
            {
                result = BDD_TARGET_SD_CARD_UNSUPPORTED;
            }
        }
    }
    SdCard_End();
    return result;
}

static inline void SdCard_Receive(uint8_t* bytes, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        bytes[i] = BddTargetBoardSdCardBus_Exchange(IDLE_LINE);
    }
}

/* ACMD41, which is CMD55 and then CMD41, asked until the card says it has
 * finished starting. */
static enum BddTargetSdCardResult SdCard_LeaveIdle(void)
{
    enum BddTargetSdCardResult result = BDD_TARGET_SD_CARD_OK;
    uint8_t answer = ANSWER_IDLE;
    uint32_t since = BddTargetBoardSdCardBus_Milliseconds();
    bool expired = false;
    while ((result == BDD_TARGET_SD_CARD_OK) && (answer == (uint8_t) ANSWER_IDLE) && !expired)
    {
        /* As in SdCard_WaitUntilReady: asked once more after the time allowed. */
        expired = SdCard_HasPassed(since, START_TIMEOUT_MILLISECONDS);
        result = SdCard_Command(APP_CMD, 0U, &answer);
        if (result == BDD_TARGET_SD_CARD_OK)
        {
            result = SdCard_Command(SD_SEND_OP_COND, HOST_HIGH_CAPACITY, &answer);
        }
    }
    if ((result == BDD_TARGET_SD_CARD_OK) && (answer == (uint8_t) ANSWER_IDLE))
    {
        result = BDD_TARGET_SD_CARD_TIMED_OUT;
    }
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_Judge(answer, ANSWER_READY);
    }
    return result;
}

/* A high capacity card is addressed by the sector. A standard capacity one is
 * addressed by the byte, and is told to use sectors of 512 of them. */
static enum BddTargetSdCardResult SdCard_ReadAddressing(void)
{
    uint8_t answer = IDLE_LINE;
    enum BddTargetSdCardResult result = SdCard_Begin(READ_OCR, 0U, &answer);
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_Judge(answer, ANSWER_READY);
    }
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        uint8_t ocr[OCR_LENGTH] = {0};
        SdCard_Receive(ocr, OCR_LENGTH);
        blockAddressed = (ocr[0] & (uint8_t) OCR_HIGH_CAPACITY) != 0U;
    }
    SdCard_End();
    if ((result == BDD_TARGET_SD_CARD_OK) && !blockAddressed)
    {
        result = SdCard_Command(SET_BLOCKLEN, BDD_TARGET_SD_CARD_SECTOR_SIZE, &answer);
        if (result == BDD_TARGET_SD_CARD_OK)
        {
            result = SdCard_Judge(answer, ANSWER_READY);
        }
    }
    return result;
}

static enum BddTargetSdCardResult SdCard_ReadCapacity(void)
{
    uint8_t answer = IDLE_LINE;
    uint8_t csd[CSD_LENGTH] = {0};
    enum BddTargetSdCardResult result = SdCard_Begin(SEND_CSD, 0U, &answer);
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_Judge(answer, ANSWER_READY);
    }
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_ReceiveBlock(csd, CSD_LENGTH);
    }
    SdCard_End();
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        sectorCount = SdCard_SectorsInCsd(csd);
    }
    return result;
}

/* A block comes after a token, however long the card takes to find it, and
 * with a CRC after it. A card that cannot send the block sends an error token
 * in the token's place. */
static enum BddTargetSdCardResult SdCard_ReceiveBlock(uint8_t* bytes, size_t count)
{
    enum BddTargetSdCardResult result = BDD_TARGET_SD_CARD_OK;
    uint32_t since = BddTargetBoardSdCardBus_Milliseconds();
    uint8_t token = BddTargetBoardSdCardBus_Exchange(IDLE_LINE);
    bool expired = false;
    while ((token == (uint8_t) IDLE_LINE) && !expired)
    {
        /* As in SdCard_WaitUntilReady: asked once more after the time allowed. */
        expired = SdCard_HasPassed(since, READ_TIMEOUT_MILLISECONDS);
        token = BddTargetBoardSdCardBus_Exchange(IDLE_LINE);
    }
    if (token == (uint8_t) IDLE_LINE)
    {
        result = BDD_TARGET_SD_CARD_TIMED_OUT;
    }
    else if (token != (uint8_t) DATA_TOKEN)
    {
        result = BDD_TARGET_SD_CARD_REFUSED;
    }
    else
    {
        uint8_t crc[CRC_LENGTH] = {0};
        SdCard_Receive(bytes, count);
        SdCard_Receive(crc, CRC_LENGTH);
        uint16_t sent = (uint16_t) (((uint16_t) crc[0] << 8U) | (uint16_t) crc[1]);
        if (sent != SdCard_Crc16(bytes, count))
        {
            result = BDD_TARGET_SD_CARD_CRC_FAILED;
        }
    }
    return result;
}

/* Section 4.5: x^16 + x^12 + x^5 + 1 over the block. */
static uint16_t SdCard_Crc16(const uint8_t* bytes, size_t count)
{
    uint16_t crc = 0U;
    for (size_t i = 0; i < count; i++)
    {
        crc ^= (uint16_t) ((uint16_t) bytes[i] << 8U);
        for (uint8_t bit = 0U; bit < 8U; bit++)
        {
            bool carry = (crc & 0x8000U) != 0U;
            crc = (uint16_t) (crc << 1U);
            if (carry)
            {
                crc ^= 0x1021U;
            }
        }
    }
    return crc;
}

/* Section 5.3. The second version of the structure, which every high capacity
 * card has, holds the capacity in units of 512 KiB less one. The first holds
 * a size, a multiplier and a block length to work it out from. */
static inline uint32_t SdCard_SectorsInCsd(const uint8_t* csd)
{
    uint32_t sectors = 0U;
    if ((csd[0] >> 6U) == 1U)
    {
        uint32_t size = (((uint32_t) csd[7] & 0x3FU) << 16U) | ((uint32_t) csd[8] << 8U) | (uint32_t) csd[9];
        sectors = (size + 1U) << 10U;
    }
    else
    {
        uint32_t blockLength = (uint32_t) csd[5] & 0x0FU;
        uint32_t size = (((uint32_t) csd[6] & 0x03U) << 10U) | ((uint32_t) csd[7] << 2U) | ((uint32_t) csd[8] >> 6U);
        uint32_t multiplier = (((uint32_t) csd[9] & 0x03U) << 1U) | ((uint32_t) csd[10] >> 7U);
        /* The block length is 512 bytes or more, so the shift is not negative. */
        sectors = (size + 1U) << ((multiplier + 2U + blockLength) - 9U);
    }
    return sectors;
}

void BddTargetSdCard_Stop(void)
{
    started = false;
    blockAddressed = false;
    sectorCount = 0U;
    repeats = 0U;
}

bool BddTargetSdCard_IsStarted(void)
{
    return started;
}

uint32_t BddTargetSdCard_SectorCount(void)
{
    return sectorCount;
}

uint32_t BddTargetSdCard_Repeats(void)
{
    return repeats;
}

enum BddTargetSdCardResult BddTargetSdCard_Read(uint32_t sector, uint8_t* buffer)
{
    enum BddTargetSdCardResult result = BDD_TARGET_SD_CARD_NOT_STARTED;
    if (started)
    {
        result = SdCard_ReadOnce(sector, buffer);
        for (uint32_t attempt = 1U;
             (attempt < (uint32_t) TRANSFER_ATTEMPTS) && (result == BDD_TARGET_SD_CARD_CRC_FAILED);
             attempt++)
        {
            repeats++;
            result = SdCard_ReadOnce(sector, buffer);
        }
    }
    return result;
}

static enum BddTargetSdCardResult SdCard_ReadOnce(uint32_t sector, uint8_t* buffer)
{
    uint8_t answer = IDLE_LINE;
    enum BddTargetSdCardResult result = SdCard_Begin(READ_SINGLE_BLOCK, SdCard_AddressOf(sector), &answer);
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_Judge(answer, ANSWER_READY);
    }
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_ReceiveBlock(buffer, BDD_TARGET_SD_CARD_SECTOR_SIZE);
    }
    SdCard_End();
    return result;
}

static inline uint32_t SdCard_AddressOf(uint32_t sector)
{
    return blockAddressed ? sector : (sector * (uint32_t) BDD_TARGET_SD_CARD_SECTOR_SIZE);
}

/* A sector written twice holds what it was sent, so sending one again is safe
 * whichever end the CRC failed at. */
enum BddTargetSdCardResult BddTargetSdCard_Write(uint32_t sector, const uint8_t* buffer)
{
    enum BddTargetSdCardResult result = BDD_TARGET_SD_CARD_NOT_STARTED;
    if (started)
    {
        result = SdCard_WriteOnce(sector, buffer);
        for (uint32_t attempt = 1U;
             (attempt < (uint32_t) TRANSFER_ATTEMPTS) && (result == BDD_TARGET_SD_CARD_CRC_FAILED);
             attempt++)
        {
            repeats++;
            result = SdCard_WriteOnce(sector, buffer);
        }
    }
    return result;
}

static enum BddTargetSdCardResult SdCard_WriteOnce(uint32_t sector, const uint8_t* buffer)
{
    uint8_t answer = IDLE_LINE;
    enum BddTargetSdCardResult result = SdCard_Begin(WRITE_BLOCK, SdCard_AddressOf(sector), &answer);
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_Judge(answer, ANSWER_READY);
    }
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_SendBlock(buffer, BDD_TARGET_SD_CARD_SECTOR_SIZE);
    }
    SdCard_End();
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        result = SdCard_CheckStatus();
    }
    return result;
}

/* The card says at once whether it took the block. It is then busy until it
 * has stored it, which the next command waits out. */
static enum BddTargetSdCardResult SdCard_SendBlock(const uint8_t* bytes, size_t count)
{
    enum BddTargetSdCardResult result = BDD_TARGET_SD_CARD_OK;
    uint16_t crc = SdCard_Crc16(bytes, count);
    (void) BddTargetBoardSdCardBus_Exchange(IDLE_LINE);
    (void) BddTargetBoardSdCardBus_Exchange(DATA_TOKEN);
    for (size_t i = 0; i < count; i++)
    {
        (void) BddTargetBoardSdCardBus_Exchange(bytes[i]);
    }
    (void) BddTargetBoardSdCardBus_Exchange((uint8_t) (crc >> 8U));
    (void) BddTargetBoardSdCardBus_Exchange((uint8_t) crc);
    uint8_t response = BddTargetBoardSdCardBus_Exchange(IDLE_LINE);
    uint8_t said = response & (uint8_t) DATA_RESPONSE_MASK;
    if (said == (uint8_t) DATA_CRC_ERROR)
    {
        result = BDD_TARGET_SD_CARD_CRC_FAILED;
    }
    else if (said != (uint8_t) DATA_ACCEPTED)
    {
        result = BDD_TARGET_SD_CARD_WRITE_FAILED;
    }
    else
    {
        /* Taken. Whether it was stored is in the card's status. */
    }
    return result;
}

/* A card can take a block and then fail to store it, and says so only in its
 * status, which is two bytes that are both zero when nothing is wrong. Asking
 * for it waits until the card has finished storing the block. */
static enum BddTargetSdCardResult SdCard_CheckStatus(void)
{
    uint8_t first = IDLE_LINE;
    enum BddTargetSdCardResult result = SdCard_Begin(SEND_STATUS, 0U, &first);
    if (result == BDD_TARGET_SD_CARD_OK)
    {
        uint8_t second = BddTargetBoardSdCardBus_Exchange(IDLE_LINE);
        result = SdCard_Judge(first, ANSWER_READY);
        if ((result == BDD_TARGET_SD_CARD_REFUSED) || ((result == BDD_TARGET_SD_CARD_OK) && (second != 0U)))
        {
            result = BDD_TARGET_SD_CARD_WRITE_FAILED;
        }
    }
    SdCard_End();
    return result;
}
