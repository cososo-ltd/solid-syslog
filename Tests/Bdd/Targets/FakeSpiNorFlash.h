#ifndef FAKE_SPI_NOR_FLASH_H
#define FAKE_SPI_NOR_FLASH_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

/* A SPI NOR flash, and the bus it is on, for the tests of the flash driver.
 * It defines the BddTargetBoardFlashBus_* functions the driver calls.
 *
 * It behaves as a Winbond W25Q128 would until a test changes one of the
 * members below. It holds what is programmed into it as a real part does: a
 * program clears bits and cannot set them, one that runs past the end of a
 * page goes round to the page's start, and neither a program nor an erase
 * happens unless writing was enabled just before it. While it is busy it
 * answers nothing but a request for its status. */
namespace FakeSpiNorFlash
{
enum
{
    PAGE_BYTES = 256,
    SECTOR_BYTES = 4096,
    JEDEC_ID_LENGTH = 3
};

struct Settings
{
    bool present = true;
    /* The line held low, as a part that is not powered may leave it. */
    bool lineHeldLow = false;
    /* Maker, memory type, and capacity as a power of two. */
    std::array<uint8_t, JEDEC_ID_LENGTH> jedecId = {0xEF, 0x40, 0x18};
    /* Requests for its status answered "busy" at power-up, as a part reset
     * in the middle of an erase would answer them. */
    int busyStatusReadsAtStart = 0;
    /* Requests for its status answered "busy" after each program or erase. */
    int busyStatusReads = 2;
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

/* Back to a part just powered and wholly erased: default settings, no history. */
void Reset();

/* Puts bytes into the part as they are, with no erase needed first. */
void Store(uint32_t address, const std::vector<uint8_t>& bytes);
std::vector<uint8_t> Stored(uint32_t address, size_t count);

/* The first byte of every command that arrived while the part was not busy, in order. */
const std::vector<uint8_t>& Commands();
int CountOf(uint8_t command);
/* Commands other than a request for status that arrived while it was busy,
 * which a real part ignores. */
int CommandsWhileBusy();
/* Every page program the part carried out, in order. */
const std::vector<Program>& Programs();
/* The address of every sector erase the part carried out, as it was sent. */
const std::vector<uint32_t>& Erases();

bool IsBusy();
bool IsSelected();
/* Whether the bus had been taken when the first command arrived. */
bool BusWasTakenBeforeFirstCommand();
size_t Exchanges();
} // namespace FakeSpiNorFlash

#endif /* FAKE_SPI_NOR_FLASH_H */
