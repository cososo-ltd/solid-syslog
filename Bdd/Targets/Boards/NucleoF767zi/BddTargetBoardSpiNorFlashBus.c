/* The NUCLEO-F767ZI's flash bus behind the board seam - see
 * BddTargetBoardSpiNorFlashBus.h. The W25Q128 is on SPI4, which BoardSpi4.c
 * drives.
 *
 * BoardSpi4_Exchange gives up on a byte the peripheral does not deliver, so
 * every call here returns in bounded time. It reads such a byte as an idle
 * line and does not say that it gave up, so Write and Read always answer
 * true: a part that has stopped answering shows in what the flash reads of
 * its status. */

#include "BddTargetBoardSpiNorFlashBus.h"

#include "BddTargetOsPrimitives.h"
#include "Board.h"
#include "BoardSpi4.h"
#include "BoardSpi4Flash.h"
#include "SolidSyslogSpiNorFlashBus.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
    /* Clocked out during a read. The part ignores it. */
    READ_FILL = 0x00
};

static void FlashBus_Select(struct SolidSyslogSpiNorFlashBus* base);
static void FlashBus_Deselect(struct SolidSyslogSpiNorFlashBus* base);
static bool FlashBus_Write(struct SolidSyslogSpiNorFlashBus* base, const void* buffer, size_t count);
static bool FlashBus_Read(struct SolidSyslogSpiNorFlashBus* base, void* buffer, size_t count);
static uint32_t FlashBus_Milliseconds(struct SolidSyslogSpiNorFlashBus* base);
static void FlashBus_WhileBusy(struct SolidSyslogSpiNorFlashBus* base);

static struct SolidSyslogSpiNorFlashBus bus = {
    FlashBus_Select,
    FlashBus_Deselect,
    FlashBus_Write,
    FlashBus_Read,
    FlashBus_Milliseconds,
    FlashBus_WhileBusy,
};

struct SolidSyslogSpiNorFlashBus* BddTargetBoardSpiNorFlashBus_Take(void)
{
    BoardSpi4Flash_Take();
    return &bus;
}

static void FlashBus_Select(struct SolidSyslogSpiNorFlashBus* base)
{
    (void) base;
    BoardSpi4_Select(BOARD_SPI4_FLASH);
}

static void FlashBus_Deselect(struct SolidSyslogSpiNorFlashBus* base)
{
    (void) base;
    BoardSpi4_Deselect(BOARD_SPI4_FLASH);
}

static bool FlashBus_Write(struct SolidSyslogSpiNorFlashBus* base, const void* buffer, size_t count)
{
    (void) base;
    const uint8_t* bytes = (const uint8_t*) buffer;
    for (size_t i = 0U; i < count; i++)
    {
        (void) BoardSpi4_Exchange(bytes[i]);
    }
    return true;
}

static bool FlashBus_Read(struct SolidSyslogSpiNorFlashBus* base, void* buffer, size_t count)
{
    (void) base;
    uint8_t* bytes = (uint8_t*) buffer;
    for (size_t i = 0U; i < count; i++)
    {
        bytes[i] = BoardSpi4_Exchange(READ_FILL);
    }
    return true;
}

static uint32_t FlashBus_Milliseconds(struct SolidSyslogSpiNorFlashBus* base)
{
    (void) base;
    return Board_Milliseconds();
}

/* A yield and not a sleep. The kernel's tick is 10 ms and a page program takes
 * about 1 ms, so a sleep through each would hold the store for a tick where
 * the part needs a tenth of one. A yield returns at once where no other thread
 * is ready. Where the thread that logs is ready it runs, for as long as the
 * rest of its time slice. */
static void FlashBus_WhileBusy(struct SolidSyslogSpiNorFlashBus* base)
{
    (void) base;
    BddTargetOsPrimitives_Yield();
}
