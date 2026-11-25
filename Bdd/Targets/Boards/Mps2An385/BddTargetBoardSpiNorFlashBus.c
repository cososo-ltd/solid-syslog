/* The QEMU mps2-an385 machine's flash bus behind the board seam - see
 * BddTargetBoardSpiNorFlashBus.h. The machine has no flash, so the part is the
 * simulated one, SimulatedSpiNorFlash.h, and what it holds is a flat image on
 * the QEMU host: solidsyslog-flash.img in QEMU's working directory.
 *
 * The image outlasts a run of the target, as a part's contents outlast a
 * reset. Behave removes it so each scenario starts with an erased part: an
 * image the board had to create holds zeros, which the part reads as ones. */

#include "BddTargetBoardSpiNorFlashBus.h"

#include "BddTargetOsPrimitives.h"
#include "SemihostingFile.h"
#include "SimulatedSpiNorFlash.h"
#include "SolidSyslogSpiNorFlashBus.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
    /* The simulated part's 16 MiB. */
    FLASH_IMAGE_BYTES = 16 * 1024 * 1024
};

static const char FLASH_IMAGE_PATH[] = "solidsyslog-flash.img";

static bool FlashImage_Read(struct SimulatedSpiNorFlashMedium* base, uint32_t offset, void* buffer, size_t count);
static bool FlashImage_IsOpen(void);
static bool FlashImage_Write(
    struct SimulatedSpiNorFlashMedium* base,
    uint32_t offset,
    const void* buffer,
    size_t count
);
static uint32_t FlashBus_Milliseconds(void);
static void FlashBus_WhileBusy(struct SolidSyslogSpiNorFlashBus* base);

static struct SimulatedSpiNorFlashMedium image = {
    FlashImage_Read,
    FlashImage_Write,
};

static int imageHandle = -1;

/* NULL until the part is first powered. */
static struct SolidSyslogSpiNorFlashBus* bus;

/* The part is powered once, and stays as it is from one call to the next, as
 * a part on a board does while the target runs. */
struct SolidSyslogSpiNorFlashBus* BddTargetBoardSpiNorFlashBus_Take(void)
{
    if (bus == NULL)
    {
        const struct SimulatedSpiNorFlashConfig config = {&image, FlashBus_Milliseconds};
        bus = SimulatedSpiNorFlash_PowerUp(&config);
        bus->WhileBusy = FlashBus_WhileBusy;
    }
    return bus;
}

static bool FlashImage_Read(struct SimulatedSpiNorFlashMedium* base, uint32_t offset, void* buffer, size_t count)
{
    (void) base;
    return FlashImage_IsOpen() && (SemihostingFile_ReadAt(imageHandle, (int) offset, buffer, (int) count) == 0);
}

/* Opened at the first use and tried again at each one after, so an image the
 * host could not give shows as a bus that fails and not as a part that hangs. */
static bool FlashImage_IsOpen(void)
{
    if (imageHandle < 0)
    {
        imageHandle = SemihostingFile_OpenImage(FLASH_IMAGE_PATH, (int) FLASH_IMAGE_BYTES);
    }
    return imageHandle >= 0;
}

static bool FlashImage_Write(struct SimulatedSpiNorFlashMedium* base, uint32_t offset, const void* buffer, size_t count)
{
    (void) base;
    return FlashImage_IsOpen() && (SemihostingFile_WriteAt(imageHandle, (int) offset, buffer, (int) count) == 0);
}

/* The kernel's count, which moves a tick at a time, so the part is busy until
 * the next tick at the least: up to 10 ms after a page program, where the
 * part's own time is 1 ms. Only differences between two readings are used, so
 * it is free to wrap. */
static uint32_t FlashBus_Milliseconds(void)
{
    return (uint32_t) BddTargetOsPrimitives_UptimeMilliseconds();
}

/* A yield, so that a thread of the same priority runs while the part is
 * busy. */
static void FlashBus_WhileBusy(struct SolidSyslogSpiNorFlashBus* base)
{
    (void) base;
    BddTargetOsPrimitives_Yield();
}
