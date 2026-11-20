#include "SpiNorFlashPartFake.h"

#include "SolidSyslogSpiNorFlashBus.h"

#include <algorithm>
#include <map>
#include <memory>

namespace
{
enum
{
    IDLE_LINE = 0xFF,
    LOW_LINE = 0x00,
    ERASED = 0xFF,
    ADDRESS_LENGTH = 3,
    COMMAND_AND_ADDRESS = 1 + ADDRESS_LENGTH,
    /* The request for SFDP is followed by one byte the part ignores. */
    SFDP_REQUEST_LENGTH = COMMAND_AND_ADDRESS + 1,

    PAGE_PROGRAM = 0x02,
    READ_DATA = 0x03,
    READ_STATUS = 0x05,
    WRITE_ENABLE = 0x06,
    READ_SFDP = 0x5A,
    READ_JEDEC_ID = 0x9F,

    STATUS_BUSY = 0x01,
    STATUS_WRITE_ENABLED = 0x02,

    SFDP_HEADER_ADDRESS = 0x00,
    SFDP_TABLE_ADDRESS = 0x80
};

/* Sixteen bytes to a row, as the part was dumped. */
// clang-format off
const std::array<uint8_t, 16> W25Q128JV_SFDP_HEADER = {0x53, 0x46, 0x44, 0x50, 0x05, 0x01, 0x00, 0xFF,
                                                        0x00, 0x05, 0x01, 0x10, 0x80, 0x00, 0x00, 0xFF};

const std::array<uint8_t, 64> W25Q128JV_SFDP_TABLE = {
    0xE5, 0x20, 0xF9, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x44, 0xEB, 0x08, 0x6B, 0x08, 0x3B, 0x42, 0xBB,
    0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x40, 0xEB, 0x0C, 0x20, 0x0F, 0x52,
    0x10, 0xD8, 0x00, 0x00, 0x36, 0x02, 0xA6, 0x00, 0x82, 0xEA, 0x14, 0xC9, 0xE9, 0x63, 0x76, 0x33,
    0x7A, 0x75, 0x7A, 0x75, 0xF7, 0xA2, 0xD5, 0x5C, 0x19, 0xF7, 0x4D, 0xFF, 0xE9, 0x30, 0xF8, 0x80};

// clang-format on

struct State
{
    bool selected = false;
    bool writeEnabled = false;
    /* A program or an erase was begun and writing has not yet been disabled
     * for its finishing. */
    bool writeInProgress = false;
    /* How many commands until the part stops answering. Zero is never. */
    int commandsUntilQuiet = 0;
    bool quiet = false;
    /* How many calls until the one that fails. Zero is none. */
    int writesUntilFailure = 0;
    int readsUntilFailure = 0;
    uint32_t milliseconds = 0;
    uint32_t busyUntil = 0;
    /* What has been written since the select fell, and how many bytes have
     * been read since then. */
    std::vector<uint8_t> written;
    size_t bytesRead = 0;
    bool ignoringCommand = false;
    std::vector<uint8_t> commands;
    int commandsWhileBusy = 0;
    int transfersWhileDeselected = 0;
    std::vector<SpiNorFlashPartFake::Program> programs;
    std::vector<uint32_t> erases;
    /* Only the bytes that are not erased, so that a part of 16 MiB costs a
     * test nothing. */
    std::map<uint32_t, uint8_t> cells;
};

/* Held for one test and no longer. An empty container still owns memory in
 * some standard libraries, so state that outlived the test would show as a
 * leak in every one. */
std::unique_ptr<State> state;

uint8_t ByteAt(uint32_t address)
{
    const auto found = state->cells.find(address);
    return (found == state->cells.end()) ? static_cast<uint8_t>(ERASED) : found->second;
}

void SetByteAt(uint32_t address, uint8_t value)
{
    if (value == ERASED)
    {
        state->cells.erase(address);
    }
    else
    {
        state->cells[address] = value;
    }
}

bool IsBusyNow()
{
    return SpiNorFlashPartFake::settings.staysBusy ||
           (state->milliseconds < SpiNorFlashPartFake::settings.busyAtPowerUpMilliseconds) ||
           (state->milliseconds < state->busyUntil);
}

uint32_t AddressIn(const std::vector<uint8_t>& frame)
{
    return (static_cast<uint32_t>(frame[1]) << 16U) | (static_cast<uint32_t>(frame[2]) << 8U) |
           static_cast<uint32_t>(frame[3]);
}

/* A part disables writing as a program or an erase finishes. */
void FinishAWriteThatHasHadItsTime()
{
    if (state->writeInProgress && !IsBusyNow())
    {
        state->writeInProgress = false;
        state->writeEnabled = false;
    }
}

uint8_t StatusByte()
{
    FinishAWriteThatHasHadItsTime();
    uint8_t status = SpiNorFlashPartFake::settings.status & static_cast<uint8_t>(~(STATUS_BUSY | STATUS_WRITE_ENABLED));
    if (IsBusyNow())
    {
        status |= STATUS_BUSY;
    }
    if (state->writeEnabled)
    {
        status |= STATUS_WRITE_ENABLED;
    }
    return status;
}

/* The first byte of a command: a busy part takes only a request for status. */
void BeginCommand(uint8_t command)
{
    FinishAWriteThatHasHadItsTime();
    if (IsBusyNow() && (command != READ_STATUS))
    {
        state->commandsWhileBusy++;
        state->ignoringCommand = true;
    }
    else
    {
        state->commands.push_back(command);
    }
}

void TakeByte(uint8_t out)
{
    if (state->written.empty())
    {
        BeginCommand(out);
    }
    state->written.push_back(out);
}

uint8_t GiveByte()
{
    uint8_t in = IDLE_LINE;
    if (!state->written.empty() && !state->ignoringCommand)
    {
        const uint8_t command = state->written[0];
        const auto sent = static_cast<uint32_t>(state->bytesRead);
        if (command == READ_STATUS)
        {
            in = StatusByte();
        }
        else if (command == READ_JEDEC_ID)
        {
            in = SpiNorFlashPartFake::settings.jedecId.at(sent % SpiNorFlashPartFake::JEDEC_ID_LENGTH);
        }
        else if ((command == READ_DATA) && (state->written.size() >= COMMAND_AND_ADDRESS))
        {
            in = ByteAt((AddressIn(state->written) + sent) % SpiNorFlashPartFake::settings.capacityBytes);
        }
        else if ((command == READ_SFDP) && (state->written.size() >= SFDP_REQUEST_LENGTH) &&
                 SpiNorFlashPartFake::settings.answersSfdp)
        {
            in = SpiNorFlashPartFake::settings.sfdp.at(
                (AddressIn(state->written) + sent) % SpiNorFlashPartFake::SFDP_BYTES
            );
        }
        else
        {
            /* Nothing to send for this command, or not all of it has arrived. */
        }
    }
    state->bytesRead++;
    return in;
}

void CarryOutProgram(const std::vector<uint8_t>& frame)
{
    const uint32_t pageBytes = SpiNorFlashPartFake::settings.pageBytes;
    const uint32_t address = AddressIn(frame) % SpiNorFlashPartFake::settings.capacityBytes;
    const uint32_t page = address - (address % pageBytes);
    const size_t length = frame.size() - COMMAND_AND_ADDRESS;
    for (size_t i = 0; i < length; i++)
    {
        const uint32_t at = page + static_cast<uint32_t>((address - page + i) % pageBytes);
        SetByteAt(at, ByteAt(at) & frame[COMMAND_AND_ADDRESS + i]);
    }
    state->programs.push_back({address, length});
    state->busyUntil = state->milliseconds + SpiNorFlashPartFake::settings.programMilliseconds;
}

void CarryOutErase(const std::vector<uint8_t>& frame)
{
    const uint32_t eraseBytes = SpiNorFlashPartFake::settings.eraseBytes;
    const uint32_t address = AddressIn(frame) % SpiNorFlashPartFake::settings.capacityBytes;
    const uint32_t first = address - (address % eraseBytes);
    state->cells.erase(state->cells.lower_bound(first), state->cells.lower_bound(first + eraseBytes));
    state->erases.push_back(address);
    state->busyUntil = state->milliseconds + SpiNorFlashPartFake::settings.eraseMilliseconds;
}

/* A part does what it was told when its select rises. */
void FinishCommand()
{
    const std::vector<uint8_t> frame = state->written;
    const bool ignored = state->ignoringCommand;
    state->written.clear();
    state->bytesRead = 0;
    state->ignoringCommand = false;
    if (frame.empty())
    {
        return;
    }
    if (state->commandsUntilQuiet > 0)
    {
        state->commandsUntilQuiet--;
        state->quiet = (state->commandsUntilQuiet == 0);
    }
    if (ignored)
    {
        return;
    }
    const uint8_t command = frame[0];
    const bool isProgram = (command == PAGE_PROGRAM) && (frame.size() > COMMAND_AND_ADDRESS);
    const bool isErase =
        (command == SpiNorFlashPartFake::settings.eraseCommand) && (frame.size() == COMMAND_AND_ADDRESS);
    if (command == WRITE_ENABLE)
    {
        state->writeEnabled = !SpiNorFlashPartFake::settings.ignoresWriteEnable;
    }
    else if (isProgram || isErase)
    {
        if (state->writeEnabled && isProgram)
        {
            CarryOutProgram(frame);
        }
        else if (state->writeEnabled)
        {
            CarryOutErase(frame);
        }
        else
        {
            /* Writing was not enabled, so the part does nothing. */
        }
        state->writeInProgress = state->writeEnabled;
    }
    else
    {
        /* Nothing is left to do for a command that only reads. */
    }
}

bool PartIsListening()
{
    return SpiNorFlashPartFake::settings.present && !state->quiet && state->selected;
}

/* True for the call a countdown reaches one on. */
bool IsTheCallThatFails(int& callsUntilFailure)
{
    const bool fails = (callsUntilFailure == 1);
    if (callsUntilFailure > 0)
    {
        callsUntilFailure--;
    }
    return fails;
}

void CountATransfer()
{
    if (!state->selected)
    {
        state->transfersWhileDeselected++;
    }
}

extern "C"
{
    void Select(struct SolidSyslogSpiNorFlashBus* base)
    {
        (void) base;
        state->selected = true;
    }

    void Deselect(struct SolidSyslogSpiNorFlashBus* base)
    {
        (void) base;
        if (PartIsListening())
        {
            FinishCommand();
        }
        state->selected = false;
    }

    bool Write(struct SolidSyslogSpiNorFlashBus* base, const void* buffer, size_t count)
    {
        (void) base;
        CountATransfer();
        const bool sent = !IsTheCallThatFails(state->writesUntilFailure);
        if (sent && PartIsListening())
        {
            const auto* bytes = static_cast<const uint8_t*>(buffer);
            std::for_each(bytes, bytes + count, TakeByte);
        }
        return sent;
    }

    bool Read(struct SolidSyslogSpiNorFlashBus* base, void* buffer, size_t count)
    {
        (void) base;
        CountATransfer();
        const bool received = !IsTheCallThatFails(state->readsUntilFailure);
        if (received)
        {
            auto* bytes = static_cast<uint8_t*>(buffer);
            std::generate(
                bytes,
                bytes + count,
                []()
                {
                    uint8_t in = IDLE_LINE;
                    if (SpiNorFlashPartFake::settings.lineHeldLow)
                    {
                        in = LOW_LINE;
                    }
                    else if (PartIsListening())
                    {
                        in = GiveByte();
                    }
                    else
                    {
                        /* Nothing drives the line. */
                    }
                    return in;
                }
            );
        }
        return received;
    }

    /* Each reading moves the time on, so a timed wait in a driver always
     * ends. */
    uint32_t MillisecondsNow(struct SolidSyslogSpiNorFlashBus* base)
    {
        (void) base;
        const uint32_t now = state->milliseconds;
        state->milliseconds += SpiNorFlashPartFake::settings.millisecondsPerReading;
        return now;
    }
}

SolidSyslogSpiNorFlashBus bus = {Select, Deselect, Write, Read, MillisecondsNow};
} // namespace

namespace SpiNorFlashPartFake
{
Settings settings;

SfdpTable W25Q128JvSfdp()
{
    SfdpTable table = {};
    table.fill(ERASED);
    std::copy(W25Q128JV_SFDP_HEADER.begin(), W25Q128JV_SFDP_HEADER.end(), table.begin() + SFDP_HEADER_ADDRESS);
    std::copy(W25Q128JV_SFDP_TABLE.begin(), W25Q128JV_SFDP_TABLE.end(), table.begin() + SFDP_TABLE_ADDRESS);
    return table;
}

void Reset()
{
    settings = Settings();
    state = std::make_unique<State>();
}

void Release()
{
    state.reset();
}

SolidSyslogSpiNorFlashBus* Bus()
{
    return &bus;
}

void FailNextWrite()
{
    FailWriteAt(1);
}

void FailNextRead()
{
    FailReadAt(1);
}

void FailWriteAt(int call)
{
    state->writesUntilFailure = call;
}

void FailReadAt(int call)
{
    state->readsUntilFailure = call;
}

void GoQuietAfter(int commands)
{
    state->commandsUntilQuiet = commands;
    state->quiet = (commands == 0);
}

void Store(uint32_t address, const std::vector<uint8_t>& bytes)
{
    for (size_t i = 0; i < bytes.size(); i++)
    {
        SetByteAt(address + static_cast<uint32_t>(i), bytes[i]);
    }
}

std::vector<uint8_t> Stored(uint32_t address, size_t count)
{
    std::vector<uint8_t> bytes(count);
    for (size_t i = 0; i < count; i++)
    {
        bytes[i] = ByteAt(address + static_cast<uint32_t>(i));
    }
    return bytes;
}

const std::vector<uint8_t>& Commands()
{
    return state->commands;
}

int CountOf(uint8_t command)
{
    return static_cast<int>(std::count(state->commands.begin(), state->commands.end(), command));
}

int CommandsWhileBusy()
{
    return state->commandsWhileBusy;
}

const std::vector<Program>& Programs()
{
    return state->programs;
}

const std::vector<uint32_t>& Erases()
{
    return state->erases;
}

bool IsBusy()
{
    return IsBusyNow();
}

bool IsSelected()
{
    return state->selected;
}

int TransfersWhileDeselected()
{
    return state->transfersWhileDeselected;
}

uint32_t Milliseconds()
{
    return state->milliseconds;
}
} // namespace SpiNorFlashPartFake
