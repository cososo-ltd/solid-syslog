#include "FakeSdCard.h"

#include "BddTargetBoardSdCardBus.h"

#include <algorithm>
#include <map>
#include <memory>

namespace
{
enum
{
    IDLE_LINE = 0xFF,
    BUSY_LINE = 0x00,
    COMMAND_LENGTH = 6,
    START_BITS = 0x40,
    START_BITS_MASK = 0xC0,
    INDEX_MASK = 0x3F,
    DATA_TOKEN = 0xFE,

    ANSWER_READY = 0x00,
    ANSWER_IDLE = 0x01,
    ANSWER_ILLEGAL_COMMAND = 0x04,
    ANSWER_CRC_ERROR = 0x08,

    DATA_ACCEPTED = 0xE5,
    DATA_CRC_ERROR = 0xEB,

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
    CRC_ON_OFF = 59
};

struct State
{
    bool selected = false;
    bool idle = false;
    bool crcOn = false;
    bool applicationCommandNext = false;
    int goIdlesSeen = 0;
    int startPollsSeen = 0;
    bool takingBlock = false;
    bool blockTokenSeen = false;
    uint32_t blockSector = 0;
    int busyBytes = 0;
    int clocksBeforeFirstCommand = 0;
    size_t exchanges = 0;
    uint32_t milliseconds = 0;
    std::vector<uint8_t> incoming;
    /* What the card has still to send, from outgoingSent on. A vector and
         * not a deque, which holds memory even when empty and so would show
         * as a leak in every test. */
    std::vector<uint8_t> outgoing;
    size_t outgoingSent = 0;
    std::vector<uint8_t> block;
    std::vector<FakeSdCard::Command> commands;
    std::vector<uint8_t> firstFrame;
    std::vector<std::string> rates;
    std::string rateAtFirstCommand;
    std::map<uint32_t, FakeSdCard::Sector> sectors;
};

/* Held for one test and no longer. An empty container still owns memory in
 * some standard libraries, so state that outlived the test would show as a
 * leak in every one. */
std::unique_ptr<State> state;

/* Worked from the specification's polynomials a bit at a time, and so
     * independently of the tables or shortcuts a driver might use. */
uint8_t Crc7(const uint8_t* bytes, size_t count)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < count; i++)
    {
        for (int bit = 7; bit >= 0; bit--)
        {
            const bool in = ((bytes[i] >> bit) & 1U) != 0U;
            const bool top = (crc & 0x40U) != 0U;
            crc = static_cast<uint8_t>((static_cast<unsigned>(crc) << 1U) & 0x7FU);
            if (in != top)
            {
                crc ^= 0x09U;
            }
        }
    }
    return crc;
}

uint16_t Crc16(const uint8_t* bytes, size_t count)
{
    uint16_t crc = 0;
    for (size_t i = 0; i < count; i++)
    {
        for (int bit = 7; bit >= 0; bit--)
        {
            const bool in = ((bytes[i] >> bit) & 1U) != 0U;
            const bool top = (crc & 0x8000U) != 0U;
            crc = static_cast<uint16_t>(crc << 1U);
            if (in != top)
            {
                crc ^= 0x1021U;
            }
        }
    }
    return crc;
}

void Answer(uint8_t answer)
{
    /* A card takes at least a byte to answer. */
    state->outgoing
        .insert(state->outgoing.end(), static_cast<size_t>(FakeSdCard::settings.idleBytesBeforeAnswer), IDLE_LINE);
    state->outgoing.push_back(answer);
}

uint8_t AnswerInState()
{
    return static_cast<uint8_t>(state->idle ? ANSWER_IDLE : ANSWER_READY);
}

void SendBlock(const uint8_t* bytes, size_t count)
{
    state->outgoing.push_back(IDLE_LINE);
    if (FakeSdCard::settings.sendsNoData)
    {
        return;
    }
    if (FakeSdCard::settings.readErrorToken != 0U)
    {
        state->outgoing.push_back(FakeSdCard::settings.readErrorToken);
        return;
    }
    state->outgoing.push_back(DATA_TOKEN);
    for (size_t i = 0; i < count; i++)
    {
        state->outgoing.push_back(bytes[i]);
    }
    uint16_t crc = Crc16(bytes, count);
    if (FakeSdCard::settings.blocksSentDamaged > 0)
    {
        FakeSdCard::settings.blocksSentDamaged--;
        crc ^= 0x0001U;
    }
    state->outgoing.push_back(static_cast<uint8_t>(crc >> 8U));
    state->outgoing.push_back(static_cast<uint8_t>(crc & 0xFFU));
}

uint32_t SectorAt(uint32_t address)
{
    return FakeSdCard::settings.highCapacity ? address : (address / FakeSdCard::SECTOR_SIZE);
}

void AnswerOperatingCondition()
{
    const bool starting = FakeSdCard::settings.neverStarts || (state->startPollsSeen < FakeSdCard::settings.startPolls);
    state->startPollsSeen++;
    if (!starting)
    {
        state->idle = false;
    }
    Answer(AnswerInState());
}

void AnswerInterfaceCondition(uint32_t argument)
{
    if (FakeSdCard::settings.versionOne)
    {
        Answer(ANSWER_ILLEGAL_COMMAND | AnswerInState());
        return;
    }
    Answer(AnswerInState());
    state->outgoing.push_back(0x00);
    state->outgoing.push_back(0x00);
    state->outgoing.push_back(static_cast<uint8_t>((argument >> 8U) & 0x0FU));
    state->outgoing.push_back(FakeSdCard::settings.checkPattern);
}

void AnswerOcr()
{
    Answer(AnswerInState());
    /* Powered up, and high capacity or not; then the voltages it takes. */
    state->outgoing.push_back(FakeSdCard::settings.highCapacity ? 0xC0 : 0x80);
    state->outgoing.push_back(0xFF);
    state->outgoing.push_back(0x80);
    state->outgoing.push_back(0x00);
}

void AnswerRead(uint32_t argument)
{
    Answer(FakeSdCard::settings.transferAnswer);
    if (FakeSdCard::settings.transferAnswer == ANSWER_READY)
    {
        const FakeSdCard::Sector& sector = state->sectors[SectorAt(argument)];
        SendBlock(sector.data(), sector.size());
    }
}

void AnswerWrite(uint32_t argument)
{
    Answer(FakeSdCard::settings.transferAnswer);
    if (FakeSdCard::settings.transferAnswer == ANSWER_READY)
    {
        state->takingBlock = true;
        state->blockTokenSeen = false;
        state->blockSector = SectorAt(argument);
        state->block.clear();
    }
}

void Obey(uint8_t index, uint32_t argument)
{
    const bool application = state->applicationCommandNext;
    state->applicationCommandNext = false;
    if (application && (index == SD_SEND_OP_COND))
    {
        AnswerOperatingCondition();
        return;
    }
    switch (index)
    {
        case GO_IDLE_STATE:
            state->idle = true;
            Answer(ANSWER_IDLE);
            break;
        case CRC_ON_OFF:
            state->crcOn = (argument & 1U) != 0U;
            Answer(AnswerInState());
            break;
        case SEND_IF_COND:
            AnswerInterfaceCondition(argument);
            break;
        case APP_CMD:
            state->applicationCommandNext = true;
            Answer(AnswerInState());
            break;
        case READ_OCR:
            AnswerOcr();
            break;
        case SET_BLOCKLEN:
            Answer(AnswerInState());
            break;
        case SEND_CSD:
            Answer(AnswerInState());
            SendBlock(FakeSdCard::settings.csd.data(), FakeSdCard::settings.csd.size());
            break;
        case READ_SINGLE_BLOCK:
            AnswerRead(argument);
            break;
        case WRITE_BLOCK:
            AnswerWrite(argument);
            break;
        case SEND_STATUS:
            Answer(AnswerInState());
            state->outgoing.push_back(FakeSdCard::settings.statusAfterWrite);
            break;
        default:
            Answer(ANSWER_ILLEGAL_COMMAND | AnswerInState());
            break;
    }
}

void TakeCommand()
{
    const std::vector<uint8_t> frame = state->incoming;
    state->incoming.clear();
    const uint8_t index = frame[0] & INDEX_MASK;
    const uint32_t argument = (static_cast<uint32_t>(frame[1]) << 24U) | (static_cast<uint32_t>(frame[2]) << 16U) |
                              (static_cast<uint32_t>(frame[3]) << 8U) | static_cast<uint32_t>(frame[4]);
    if (state->commands.empty())
    {
        state->firstFrame = frame;
        state->rateAtFirstCommand = state->rates.empty() ? "" : state->rates.back();
    }
    state->commands.push_back({index, argument});

    if ((index == GO_IDLE_STATE) && (state->goIdlesSeen < FakeSdCard::settings.goIdlesIgnored))
    {
        state->goIdlesSeen++;
        return;
    }
    /* A card checks the CRC of every command until it is in SPI mode, and
         * of CMD8 always; after that only when it has been told to. */
    const bool checked = state->crcOn || (index == GO_IDLE_STATE) || (index == SEND_IF_COND);
    const auto expected =
        static_cast<uint8_t>((static_cast<unsigned>(Crc7(frame.data(), COMMAND_LENGTH - 1)) << 1U) | 1U);
    if (checked && (frame[COMMAND_LENGTH - 1] != expected))
    {
        Answer(ANSWER_CRC_ERROR | AnswerInState());
        return;
    }
    Obey(index, argument);
}

void FinishBlock()
{
    state->takingBlock = false;
    const auto sent = static_cast<uint16_t>(
        (static_cast<unsigned>(state->block[FakeSdCard::SECTOR_SIZE]) << 8U) | state->block[FakeSdCard::SECTOR_SIZE + 1]
    );
    uint8_t response = DATA_ACCEPTED;
    if (state->crcOn && (sent != Crc16(state->block.data(), FakeSdCard::SECTOR_SIZE)))
    {
        response = DATA_CRC_ERROR;
    }
    if (FakeSdCard::settings.blocksTakenAsDamaged > 0)
    {
        FakeSdCard::settings.blocksTakenAsDamaged--;
        response = DATA_CRC_ERROR;
    }
    if (FakeSdCard::settings.writeResponse != 0U)
    {
        response = FakeSdCard::settings.writeResponse;
    }
    state->outgoing.push_back(response);
    if (response == DATA_ACCEPTED)
    {
        FakeSdCard::Sector sector = {};
        std::copy_n(state->block.begin(), sector.size(), sector.begin());
        state->sectors[state->blockSector] = sector;
        state->busyBytes = FakeSdCard::settings.busyBytesAfterWrite;
    }
}

void TakeBlockByte(uint8_t out)
{
    if (!state->blockTokenSeen)
    {
        state->blockTokenSeen = (out == DATA_TOKEN);
        return;
    }
    state->block.push_back(out);
    if (state->block.size() == (FakeSdCard::SECTOR_SIZE + 2U))
    {
        FinishBlock();
    }
}

uint8_t ExchangeSelected(uint8_t out)
{
    if (FakeSdCard::settings.stuckBusy)
    {
        return BUSY_LINE;
    }
    if (state->outgoingSent < state->outgoing.size())
    {
        return state->outgoing[state->outgoingSent++];
    }
    if (state->takingBlock)
    {
        TakeBlockByte(out);
        return IDLE_LINE;
    }
    if (FakeSdCard::settings.staysBusyAfterWrite && (state->busyBytes > 0))
    {
        return BUSY_LINE;
    }
    if (state->busyBytes > 0)
    {
        state->busyBytes--;
        return BUSY_LINE;
    }
    const bool startsCommand = (out & START_BITS_MASK) == START_BITS;
    if (!state->incoming.empty() || startsCommand)
    {
        state->incoming.push_back(out);
        if (state->incoming.size() == COMMAND_LENGTH)
        {
            TakeCommand();
        }
    }
    return IDLE_LINE;
}
} // namespace

namespace FakeSdCard
{
Settings settings;

void Reset()
{
    settings = Settings();
    state = std::make_unique<State>();
    SetHighCapacitySectors(15523840U);
}

void Release()
{
    state.reset();
}

void SetHighCapacitySectors(uint32_t sectors)
{
    const uint32_t size = (sectors / 1024U) - 1U;
    settings.csd = {};
    settings.csd[0] = 0x40; /* version 2 of the structure */
    settings.csd[7] = static_cast<uint8_t>((size >> 16U) & 0x3FU);
    settings.csd[8] = static_cast<uint8_t>((size >> 8U) & 0xFFU);
    settings.csd[9] = static_cast<uint8_t>(size & 0xFFU);
}

void SetStandardCapacityTwoGigabytes()
{
    /* Blocks of 1024 bytes, a size field of 4095 and a multiplier of 7:
         * 4096 * 512 blocks, which is 4194304 sectors of 512 bytes. */
    settings.csd = {};
    settings.csd[5] = 0x0A;
    settings.csd[6] = 0x03;
    settings.csd[7] = 0xFF;
    settings.csd[8] = 0xC0;
    settings.csd[9] = 0x03;
    settings.csd[10] = 0x80;
}

void Store(uint32_t sector, const Sector& content)
{
    state->sectors[sector] = content;
}

bool Holds(uint32_t sector)
{
    return state->sectors.count(sector) != 0U;
}

const Sector& Stored(uint32_t sector)
{
    return state->sectors[sector];
}

const std::vector<Command>& Commands()
{
    return state->commands;
}

int CountOf(uint8_t index)
{
    int count = 0;
    for (const Command& command : state->commands)
    {
        if (command.index == index)
        {
            count++;
        }
    }
    return count;
}

bool Received(uint8_t index)
{
    bool received = false;
    for (const Command& command : state->commands)
    {
        received = received || (command.index == index);
    }
    return received;
}

uint32_t ArgumentOf(uint8_t index)
{
    uint32_t argument = 0;
    for (const Command& command : state->commands)
    {
        if (command.index == index)
        {
            argument = command.argument;
        }
    }
    return argument;
}

const std::vector<uint8_t>& FirstFrame()
{
    return state->firstFrame;
}

int ClocksBeforeFirstCommand()
{
    return state->clocksBeforeFirstCommand;
}

bool IsSelected()
{
    return state->selected;
}

bool CrcIsOn()
{
    return state->crcOn;
}

const std::vector<std::string>& Rates()
{
    return state->rates;
}

const std::string& RateAtFirstCommand()
{
    return state->rateAtFirstCommand;
}

size_t Exchanges()
{
    return state->exchanges;
}
} // namespace FakeSdCard

extern "C"
{
    void BddTargetBoardSdCardBus_UseStartRate(void)
    {
        state->rates.emplace_back("start");
    }

    void BddTargetBoardSdCardBus_UseRunRate(void)
    {
        state->rates.emplace_back("run");
    }

    void BddTargetBoardSdCardBus_Select(void)
    {
        state->selected = true;
    }

    void BddTargetBoardSdCardBus_Deselect(void)
    {
        /* A card stops what it was sending when its select rises. */
        state->selected = false;
        state->outgoing.clear();
        state->outgoingSent = 0;
        state->incoming.clear();
    }

    uint8_t BddTargetBoardSdCardBus_Exchange(uint8_t out)
    {
        state->exchanges++;
        uint8_t in = IDLE_LINE;
        if (FakeSdCard::settings.present && state->selected)
        {
            in = ExchangeSelected(out);
        }
        else if (state->commands.empty())
        {
            state->clocksBeforeFirstCommand += 8;
        }
        else
        {
            /* Clocked with the select high: nothing to count. */
        }
        return in;
    }

    /* Each reading is a millisecond after the last, so a wait the driver
     * times comes to its end however little else happens. */
    uint32_t BddTargetBoardSdCardBus_Milliseconds(void)
    {
        const uint32_t now = state->milliseconds;
        state->milliseconds += FakeSdCard::settings.millisecondsPerReading;
        return now;
    }
}
