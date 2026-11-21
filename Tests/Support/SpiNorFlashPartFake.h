#ifndef SPINORFLASHPARTFAKE_H
#define SPINORFLASHPARTFAKE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

struct SolidSyslogSpiNorFlashBus;

/* A SPI NOR flash part, and the bus it is on, behind the SpiNorFlashBus seam.
 *
 * Until a test changes a setting it is the Winbond W25Q128JV the SFDP table
 * below was read from. It behaves as a part does where a driver could tell the
 * difference: a program clears bits and sets none, one that runs past the end
 * of its page goes round to the page's start, neither a program nor an erase
 * happens unless writing was enabled just before it, and while it is busy it
 * answers nothing but a request for its status. Its status shows writing
 * enabled from the enable until the program or erase that follows has
 * finished.
 *
 * The part's geometry and its SFDP table are separate settings, as they are in
 * silicon: the part erases by eraseCommand and eraseBytes whatever its table
 * says, so a driver that ignores the table, or a table that is wrong, shows.
 *
 * Time is the count Milliseconds returns, which moves on at each reading. The
 * part is busy for a time and not for a number of questions, so a driver that
 * waits without reading the time waits for ever. */
namespace SpiNorFlashPartFake
{
enum
{
    SFDP_BYTES = 256,
    JEDEC_ID_LENGTH = 3
};

using SfdpTable = std::array<uint8_t, SFDP_BYTES>;

/* The table read from a W25Q128JV on 2026-11-20: SFDP 1.5, one parameter
 * header, a basic table of 16 DWORDs at 0x80. Every byte not in the header or
 * the table reads 0xFF. */
SfdpTable W25Q128JvSfdp();

struct Settings
{
    bool present = true;
    /* The line held low, as a part that is not powered may leave it. */
    bool lineHeldLow = false;
    /* Maker, memory type, and capacity as a power of two. */
    std::array<uint8_t, JEDEC_ID_LENGTH> jedecId = {0xEF, 0x40, 0x18};
    /* A part with no SFDP leaves the line alone when asked for it. */
    bool answersSfdp = true;
    /* Read back from any address, repeating every SFDP_BYTES. */
    SfdpTable sfdp = W25Q128JvSfdp();
    /* The bits of the first status register other than busy and write enabled,
     * which the part sets itself. Bits 2 to 6 are its block protection. */
    uint8_t status = 0x00;
    uint32_t capacityBytes = 16U * 1024U * 1024U;
    uint32_t pageBytes = 256;
    uint8_t eraseCommand = 0x20;
    uint32_t eraseBytes = 4096;
    /* How long the part is busy after each. */
    uint32_t eraseMilliseconds = 50;
    uint32_t programMilliseconds = 1;
    /* Busy from power-up, as a part reset in the middle of an erase is. */
    uint32_t busyAtPowerUpMilliseconds = 0;
    bool staysBusy = false;
    /* A part that does not take the command that enables writing. */
    bool ignoresWriteEnable = false;
    /* How far the count of milliseconds moves on each time it is read. A
     * large step is a task that was switched out between two readings. */
    uint32_t millisecondsPerReading = 1;
};

extern Settings settings;

struct Program
{
    uint32_t address;
    size_t length;
};

/* Back to a part just powered and wholly erased: default settings, no history,
 * the count of milliseconds at zero. */
void Reset();

/* Gives back everything the fake holds. Nothing else may be called until the
 * next Reset. */
void Release();

/* Powers the part up with the count of milliseconds at that value and not at
 * zero, for a test of a count about to wrap. Call it before anything else. */
void StartClockAt(uint32_t milliseconds);

/* The bus the part is on. The same table every time. */
SolidSyslogSpiNorFlashBus* Bus();

/* The next call of that kind answers false, and the part sees none of it. */
void FailNextWrite();
void FailNextRead();
/* The same for the call that many from now, counting from 1. */
void FailWriteAt(int call);
void FailReadAt(int call);

/* The part takes that many more commands and then stops answering, as one
 * that has lost power does. */
void GoQuietAfter(int commands);

/* Puts bytes into the part as they are, with no erase needed first. */
void Store(uint32_t address, const std::vector<uint8_t>& bytes);
std::vector<uint8_t> Stored(uint32_t address, size_t count);

/* The first byte of every command that arrived while the part was not busy, in order. */
const std::vector<uint8_t>& Commands();
int CountOf(uint8_t command);
/* Commands other than a request for status that arrived while it was busy,
 * which a real part ignores. */
int CommandsWhileBusy();
/* Every page program the part carried out, in order, each at the address it
 * was sent. One past the end of the part is recorded as sent, though the part
 * programmed the address it wraps to. */
const std::vector<Program>& Programs();
/* The address of every erase the part carried out, as it was sent, in the
 * same way. */
const std::vector<uint32_t>& Erases();

bool IsBusy();
bool IsSelected();
/* Calls of Write and Read made while the part was not selected. */
int TransfersWhileDeselected();
/* The count of milliseconds, without moving it on. */
uint32_t Milliseconds();
} // namespace SpiNorFlashPartFake

#endif /* SPINORFLASHPARTFAKE_H */
