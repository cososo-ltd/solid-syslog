#ifndef FAKE_SD_CARD_H
#define FAKE_SD_CARD_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/* An SD card in SPI mode, and the bus it is on, for the tests of the card
 * driver. It defines the BddTargetSdCardBus_* functions the driver calls.
 *
 * It behaves as a version 2 high capacity card would until a test changes one
 * of the members below, and it holds whatever is written to it. A command
 * with a wrong CRC is answered as a card answers one. */
namespace FakeSdCard
{
enum
{
    SECTOR_SIZE = 512,
    CSD_LENGTH = 16
};

using Sector = std::array<uint8_t, SECTOR_SIZE>;

struct Command
{
    uint8_t index;
    uint32_t argument;
};

struct Settings
{
    bool present = true;
    /* A card of the first version of the specification, which does not know CMD8. */
    bool versionOne = false;
    bool highCapacity = true;
    /* The byte a card echoes back from CMD8's argument. */
    uint8_t checkPattern = 0xAA;
    /* CMD0s the card lets pass with no answer, as one just powered may. */
    int goIdlesIgnored = 0;
    /* ACMD41s answered "still starting" before the one answered "ready". */
    int startPolls = 2;
    bool neverStarts = false;
    /* Holds its data line low for ever, as a card stuck busy does. */
    bool stuckBusy = false;
    /* What it answers a read or a write command with, in place of 0x00. */
    uint8_t transferAnswer = 0x00;
    bool sendsNoData = false;
    /* Sent in place of the token that starts a block of data, when not 0. */
    uint8_t readErrorToken = 0x00;
    bool corruptsDataItSends = false;
    /* Sent in place of the response a card gives to a block it took, when not 0. */
    uint8_t writeResponse = 0x00;
    bool staysBusyAfterWrite = false;
    /* The second byte of the status a card gives after a write. */
    uint8_t statusAfterWrite = 0x00;
    std::array<uint8_t, CSD_LENGTH> csd = {};
};

extern Settings settings;

/* Back to a card just powered: default settings, nothing stored, no history. */
void Reset();

/* The CSD of a high capacity card of this many sectors, a multiple of 1024. */
void SetHighCapacitySectors(uint32_t sectors);
/* The CSD of a standard capacity card of 2 GiB, the largest there is. */
void SetStandardCapacityTwoGigabytes();

void Store(uint32_t sector, const Sector& content);
bool Holds(uint32_t sector);
const Sector& Stored(uint32_t sector);

/* Every command that arrived whole, in order. */
const std::vector<Command>& Commands();
bool Received(uint8_t index);
uint32_t ArgumentOf(uint8_t index);
/* The six bytes of the first command, as they were on the wire. */
const std::vector<uint8_t>& FirstFrame();

/* Bytes clocked with the card not selected, before its first command. */
int ClocksBeforeFirstCommand();
bool IsSelected();
bool CrcIsOn();
/* "start" and "run", in the order the driver chose them. */
const std::vector<std::string>& Rates();
/* The rate the bus was at when the first command arrived. */
const std::string& RateAtFirstCommand();
size_t Exchanges();
} // namespace FakeSdCard

#endif /* FAKE_SD_CARD_H */
