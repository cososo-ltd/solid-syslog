#include "FakeSpiNorFlash.h"

#include "BddTargetBoardFlashBus.h"

#include <algorithm>
#include <map>

namespace
{
enum
{
    IDLE_LINE = 0xFF,
    LOW_LINE = 0x00,
    ERASED = 0xFF,
    ADDRESS_LENGTH = 3,

    PAGE_PROGRAM = 0x02,
    READ_DATA = 0x03,
    READ_STATUS = 0x05,
    WRITE_ENABLE = 0x06,
    SECTOR_ERASE = 0x20,
    READ_JEDEC_ID = 0x9F,

    STATUS_BUSY = 0x01,
    STATUS_WRITE_ENABLED = 0x02
};

using Sector = std::array<uint8_t, FakeSpiNorFlash::SECTOR_SIZE>;

struct State
{
    bool selected = false;
    bool taken = false;
    bool takenBeforeFirstCommand = false;
    bool commandSeen = false;
    bool writeEnabled = false;
    bool startBusyCounted = false;
    int busyStatusReadsLeft = 0;
    size_t exchanges = 0;
    uint32_t milliseconds = 0;
    /* What has arrived since the select fell. */
    std::vector<uint8_t> incoming;
    bool ignoringCommand = false;
    uint32_t readAddress = 0;
    size_t jedecSent = 0;
    std::vector<uint8_t> commands;
    int commandsWhileBusy = 0;
    std::vector<FakeSpiNorFlash::Program> programs;
    std::vector<uint32_t> erases;
    /* Only the sectors that hold something, so that a part of 16 MiB costs a
     * test nothing. */
    std::map<uint32_t, Sector> sectors;
};

State state;

uint32_t Capacity()
{
    return static_cast<uint32_t>(1U) << FakeSpiNorFlash::settings.jedecId[2];
}

uint8_t ByteAt(uint32_t address)
{
    const auto found = state.sectors.find(address / FakeSpiNorFlash::SECTOR_SIZE);
    return (found == state.sectors.end()) ? static_cast<uint8_t>(ERASED)
                                          : found->second[address % FakeSpiNorFlash::SECTOR_SIZE];
}

uint8_t& ByteToChangeAt(uint32_t address)
{
    const uint32_t sector = address / FakeSpiNorFlash::SECTOR_SIZE;
    auto found = state.sectors.find(sector);
    if (found == state.sectors.end())
    {
        Sector erased = {};
        erased.fill(ERASED);
        found = state.sectors.emplace(sector, erased).first;
    }
    return found->second[address % FakeSpiNorFlash::SECTOR_SIZE];
}

bool IsBusyNow()
{
    return FakeSpiNorFlash::settings.staysBusy || (state.busyStatusReadsLeft > 0);
}

uint32_t AddressIn(const std::vector<uint8_t>& frame)
{
    return (static_cast<uint32_t>(frame[1]) << 16U) | (static_cast<uint32_t>(frame[2]) << 8U) |
           static_cast<uint32_t>(frame[3]);
}

uint8_t StatusByte()
{
    uint8_t status = 0;
    if (IsBusyNow())
    {
        status |= STATUS_BUSY;
        if (state.busyStatusReadsLeft > 0)
        {
            state.busyStatusReadsLeft--;
        }
    }
    if (state.writeEnabled)
    {
        status |= STATUS_WRITE_ENABLED;
    }
    return status;
}

/* The first byte of a command: a busy part takes only a request for status. */
void BeginCommand(uint8_t command)
{
    if (!state.commandSeen)
    {
        state.commandSeen = true;
        state.takenBeforeFirstCommand = state.taken;
    }
    if (IsBusyNow() && (command != READ_STATUS))
    {
        state.commandsWhileBusy++;
        state.ignoringCommand = true;
    }
    else
    {
        state.commands.push_back(command);
    }
}

uint8_t ExchangeSelected(uint8_t out)
{
    uint8_t in = IDLE_LINE;
    if (state.incoming.empty())
    {
        BeginCommand(out);
    }
    else if (!state.ignoringCommand)
    {
        const uint8_t command = state.incoming[0];
        const bool addressArrived = state.incoming.size() > ADDRESS_LENGTH;
        if (command == READ_STATUS)
        {
            in = StatusByte();
        }
        else if (command == READ_JEDEC_ID)
        {
            in = FakeSpiNorFlash::settings.jedecId[state.jedecSent % FakeSpiNorFlash::JEDEC_ID_LENGTH];
            state.jedecSent++;
        }
        else if ((command == READ_DATA) && addressArrived)
        {
            if (state.incoming.size() == (ADDRESS_LENGTH + 1U))
            {
                state.readAddress = AddressIn(state.incoming);
            }
            in = ByteAt(state.readAddress % Capacity());
            state.readAddress++;
        }
        else
        {
            /* Still taking the command in. */
        }
    }
    else
    {
        /* Busy: the line is left alone. */
    }
    state.incoming.push_back(out);
    return in;
}

/* A program that reaches the end of its page goes on from the page's start. */
void CarryOutProgram(const std::vector<uint8_t>& frame)
{
    const uint32_t address = AddressIn(frame);
    const uint32_t page = address - (address % FakeSpiNorFlash::PAGE_SIZE);
    const size_t length = frame.size() - (ADDRESS_LENGTH + 1U);
    for (size_t i = 0; i < length; i++)
    {
        const uint32_t offset = static_cast<uint32_t>((address - page + i) % FakeSpiNorFlash::PAGE_SIZE);
        ByteToChangeAt(page + offset) &= frame[ADDRESS_LENGTH + 1U + i];
    }
    state.programs.push_back({address, length});
}

void CarryOutErase(const std::vector<uint8_t>& frame)
{
    const uint32_t address = AddressIn(frame);
    state.sectors.erase(address / FakeSpiNorFlash::SECTOR_SIZE);
    state.erases.push_back(address);
}

/* A part does what it was told when its select rises. */
void FinishCommand()
{
    const std::vector<uint8_t> frame = state.incoming;
    state.incoming.clear();
    const bool ignored = state.ignoringCommand;
    state.ignoringCommand = false;
    state.jedecSent = 0;
    if (frame.empty() || ignored)
    {
        return;
    }
    const uint8_t command = frame[0];
    if (command == WRITE_ENABLE)
    {
        state.writeEnabled = !FakeSpiNorFlash::settings.ignoresWriteEnable;
    }
    else if ((command == PAGE_PROGRAM) || (command == SECTOR_ERASE))
    {
        const bool whole = (command == PAGE_PROGRAM) ? (frame.size() > (ADDRESS_LENGTH + 1U))
                                                     : (frame.size() == (ADDRESS_LENGTH + 1U));
        if (state.writeEnabled && whole)
        {
            if (command == PAGE_PROGRAM)
            {
                CarryOutProgram(frame);
            }
            else
            {
                CarryOutErase(frame);
            }
            state.busyStatusReadsLeft = FakeSpiNorFlash::settings.busyStatusReads;
        }
        state.writeEnabled = false;
    }
    else
    {
        /* Nothing is left to do for a command that only reads. */
    }
}
} // namespace

namespace FakeSpiNorFlash
{
Settings settings;

void Reset()
{
    settings = Settings();
    state = State();
}

void Store(uint32_t address, const std::vector<uint8_t>& bytes)
{
    for (size_t i = 0; i < bytes.size(); i++)
    {
        ByteToChangeAt(address + static_cast<uint32_t>(i)) = bytes[i];
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
    return state.commands;
}

int CountOf(uint8_t command)
{
    return static_cast<int>(std::count(state.commands.begin(), state.commands.end(), command));
}

int CommandsWhileBusy()
{
    return state.commandsWhileBusy;
}

const std::vector<Program>& Programs()
{
    return state.programs;
}

const std::vector<uint32_t>& Erases()
{
    return state.erases;
}

bool IsBusy()
{
    return IsBusyNow();
}

bool IsSelected()
{
    return state.selected;
}

bool BusWasTakenBeforeFirstCommand()
{
    return state.takenBeforeFirstCommand;
}

size_t Exchanges()
{
    return state.exchanges;
}
} // namespace FakeSpiNorFlash

extern "C"
{
    void BddTargetBoardFlashBus_Take(void)
    {
        state.taken = true;
    }

    void BddTargetBoardFlashBus_Select(void)
    {
        if (!state.startBusyCounted)
        {
            state.startBusyCounted = true;
            state.busyStatusReadsLeft = FakeSpiNorFlash::settings.busyStatusReadsAtStart;
        }
        state.selected = true;
    }

    void BddTargetBoardFlashBus_Deselect(void)
    {
        if (state.selected && FakeSpiNorFlash::settings.present)
        {
            FinishCommand();
        }
        state.selected = false;
    }

    uint8_t BddTargetBoardFlashBus_Exchange(uint8_t out)
    {
        state.exchanges++;
        uint8_t in = IDLE_LINE;
        if (FakeSpiNorFlash::settings.lineHeldLow)
        {
            in = LOW_LINE;
        }
        else if (FakeSpiNorFlash::settings.present && state.selected)
        {
            in = ExchangeSelected(out);
        }
        else
        {
            /* Nothing drives the line. */
        }
        return in;
    }

    /* Each reading is later than the last, so a wait the driver times comes
     * to its end however little else happens. */
    uint32_t BddTargetBoardFlashBus_Milliseconds(void)
    {
        const uint32_t now = state.milliseconds;
        state.milliseconds += FakeSpiNorFlash::settings.millisecondsPerReading;
        return now;
    }
}
