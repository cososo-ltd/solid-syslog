# SPI NOR flash setup

Wiring a flash part beneath the store. [SPI NOR flash](index.md) covers what the
pack fills and what it refuses.

## What you provide

The bus the part is on, as the functions of
[`SolidSyslogSpiNorFlashBus`](../../api/structSolidSyslogSpiNorFlashBus.md):
select, deselect, write a buffer, read a buffer, and a count of milliseconds.
A sixth, `WhileBusy`, is yours to fill or leave: it is where the pack lets you
give up the processor while the part is busy. The header states what each must
do.

Bring the bus up before you create the flash: its pins, SPI mode 0, and its
rate are yours to set.

## Build

The pack has no upstream to configure, so it compiles straight into the static
library:

```cmake
set(SOLIDSYSLOG_PLATFORMS "SpiNorFlash;<Network>;<OsPrimitives>")
```

The placeholders are whichever platforms the [capability matrix](../index.md)
says fill the rest of what your build needs. See
[naming your platforms](../../build-integration.md#cmake) for how the list is
read.

## Wiring it

Fill the table, embedded as the first member of a struct of your own if the
functions need to find anything, and hand it to Create:

```c
static struct SolidSyslogSpiNorFlashBus bus = {
    BoardFlash_Select, BoardFlash_Deselect, BoardFlash_Write, BoardFlash_Read, BoardFlash_Milliseconds,
    BoardFlash_WhileBusy /* or NULL */
};

struct SolidSyslogSpiNorFlashConfig flashConfig = {&bus};
struct SolidSyslogNorFlash* flash = SolidSyslogSpiNorFlash_Create(&flashConfig);
```

The flash then goes to
[`SolidSyslogNorFlashBlockDevice_Create`](../../api/SolidSyslogNorFlashBlockDevice_8h.md),
with the run of blocks the store may use. A block is one erase of the smallest
size the part has, commonly 4 KiB.

## What will catch you out

**Create talks to the part.** It waits for the part to be ready and reads its
SFDP table, so the bus must already work, and Create can take as long as the
wait its header states where a reset has left the part part-way through an
erase.

**Every bus call must come back.** The pack bounds how long it waits for the
part, by your count of milliseconds. It cannot bound a call that never
returns, so give each of your bus functions a time limit of its own. `Write`
and `Read` answer false when theirs passes; the others have no failure to
report.

**Service is held while the part erases.** The store erases a block as it takes
it, and the call of `SolidSyslog_Service` that met it returns only when the
part has finished. On a Winbond W25Q128JV that was 52 to 58 ms, about once for
every block of records; the most the pack allows is in
[`SolidSyslogSpiNorFlash_Create`](../../api/SolidSyslogSpiNorFlash_8h.md).

**With `WhileBusy` left NULL, nothing else runs meanwhile.** The pack waits by
asking the part its status over and over, with nothing between. Under a
pre-emptive kernel, tasks of lower priority than the one that calls Service get
no time until the erase ends, and a loop with no kernel is held for all of it.
Fill `WhileBusy` and the pack calls it between one ask and the next, which is
where you sleep, yield, or service a watchdog. Service still returns only when
the part has finished; what changes is what the processor does until then.

**`WhileBusy` is called through a page program too, and is not told which.** A
page program is far shorter than an erase and far more frequent:
[a store on NOR flash](../../roles/flash-store-costs.md) counts both for a
message. Where your kernel's shortest sleep is longer than a page program
takes, a `WhileBusy` that sleeps every time it is called slows every record the
store writes. What it does is yours to choose, so choose it for your kernel's
tick and your part's times.

**Create may call `WhileBusy`.** A part that a reset left part-way through an
erase is waited for in Create, through the same function. If yours sleeps on a
kernel, create the flash where sleeping is allowed, or have the function tell
whether the kernel is running.

**Another device on the same wires is yours to keep quiet.** Whatever it needs
before it will stay off the bus while the part is selected, do before Create,
and hold whatever lock the bus needs from select to deselect.

**The store's blocks must not be write-protected.** The pack does not look,
and never unlocks a part. If your part ships with protection set, or your
product sets it, clear it for the store's blocks in your own code before
Create.

## When it does not work

Install an error handler before you start; [error severity](../../error-severity.md)
says what each level is telling you. A refusal at Create is reported with a
detail from
[`SolidSyslogSpiNorFlashErrors`](../../api/SolidSyslogSpiNorFlashErrors_8h.md)
naming which.
