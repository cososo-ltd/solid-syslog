# NUCLEO-F767ZI board

ST's NUCLEO-F767ZI carries an STM32F767ZI, a Cortex-M7 with on-board Ethernet,
and an ST-LINK that gives flash, reset, debug and a serial port over one USB
cable. The `nucleo-f767zi-*` presets select it, the BDD target presets with
`SOLIDSYSLOG_BDD_BOARD=NucleoF767zi`.

CI builds firmware for this board and never runs it. It is run from a
development machine with the board attached, and each run is recorded against
the [target checklist](#target-checklist).
[One command](#running-the-bdd-scenarios) runs the BDD scenarios.

This directory is the board's side of the board seam.
[`../Board.cmake`](../Board.cmake) states what a board supplies to the build,
and the `BddTargetBoard*.h` headers in [`../../Common/`](../../Common/) state
what it supplies to the code.

## What is here

| Part of the seam | File | How this board fills it |
|---|---|---|
| Toolchain | `cmake/arm-none-eabi-cortex-m7.cmake` | `arm-none-eabi-gcc` for a Cortex-M7 with the hard-float ABI |
| Startup and vectors | - | ST's `startup_stm32f767xx.s` and `system_stm32f7xx.c`, compiled unmodified from the pinned tree |
| Linker script | `stm32f767zi.ld` | 2 MiB of flash at `0x08000000` and 384 KiB of RAM at `0x20020000`, the first 32 KiB for the Ethernet DMA |
| Clock, NVIC and disk facts | `BddTargetBoardConfig.h` | 216 MHz, four priority bits, an 8 MiB disk |
| Network addresses | `BddTargetBoardAddresses.h.in` | Written into the build tree at configure time, from cache variables |
| Caches, clock, console, LEDs, millisecond count | `Board.c`, `stm32f7xx_hal_conf.h` | ST's HAL. Both [caches](#caches) are on. TIM6 supplies the millisecond count, which leaves SysTick to the kernel |
| Console, exit and fault | `BddTargetBoard.c` | USART3 through `Board.c`. An exit or a fault is a line on the console followed by a halt |
| Time of day | `BddTargetBoardClock.c`, `BoardRtc.c` | The [real-time clock](#clock), read to the microsecond once `set time` has set it |
| Sector disk | `BddTargetBoardDisk.c`, `BddTargetBoardSdCardBus.c` | A [MicroSD card](#disk) on SPI4, under the shared card driver |
| NOR flash bus | `BddTargetBoardSpiNorFlashBus.c`, `BoardSpi4Flash.c` | A [W25Q128 flash](#w25q128-flash) on SPI4, as the bus under the library's SPI NOR flash pack. One image puts the shared LittleFS block device over the pack, and another the store itself |
| SPI4 | `BoardSpi4.c` | The pins, the rate and the two chip selects |
| Ethernet, under any stack | `BoardEthernet.c`, `EthernetPhy.c` | The MAC's clock, pins and interrupt, which ST's HAL Ethernet driver asks the board for, and a restart of the PHY before the MAC is reset. The link is read from the PHY's standard registers |
| Ethernet under lwIP | `Lwip/BddTargetBoardNetif.c` | A netif over ST's HAL Ethernet driver |
| Ethernet under FreeRTOS-Plus-TCP | `PlusTcp/BddTargetBoardPlusTcp.c` | Names the network interface FreeRTOS-Plus-TCP ships for the STM32 MACs, which runs over the same driver |
| CMSIS device header | `BddTargetCmsisDevice.h` | ST's own, from which the CMSIS-RTOS2 wrapper reads the core |

`Bringup/` is a separate image and not part of the seam. It has no SolidSyslog,
kernel or network stack and is built directly on `Board.c`: it prints a banner,
echoes each line it receives and blinks the green LED. It separates a fault in
the board or the toolchain from a fault in anything built on them. Its `probe`
command asks each [storage breakout](#storage-breakouts-spi4) whether it is
present, with no driver involved. Its `flash` command erases, programs and
reads back the last sector of the [flash](#w25q128-flash) through a flash
driver of its own, with no filesystem and none of the library.

ST's HAL driver and CMSIS device files are not in the repository. The cross
image pins them, and the build reads them from `STM32F7_HAL_DRIVER_PATH` and
`STM32F7_CMSIS_DEVICE_PATH`.

## Build

| Image | Preset | Build target | CI lane | Artifact |
|---|---|---|---|---|
| Bring-up | `nucleo-f767zi-bringup` | `SolidSyslogNucleoF767ziBringup` | `build-nucleo-bringup` | `solid-syslog-nucleo-f767zi-bringup` |
| [FreeRTOS + lwIP Raw](../../FreeRtosLwip/README.md) | `nucleo-f767zi-lwip` | `SolidSyslogBddTargetLwip` | `build-nucleo-freertos-lwipraw` | `solid-syslog-nucleo-f767zi-freertos-lwip` |
| [CMSIS-RTOS2 + lwIP Sockets](../../CmsisLwip/README.md) | `nucleo-f767zi-cmsis-lwip` | `SolidSyslogBddTargetCmsis` | `build-nucleo-cmsisrtos-lwipsocket` | `solid-syslog-nucleo-f767zi-cmsis-lwip` |
| [CMSIS-RTOS2 + lwIP Sockets](../../CmsisLwip/README.md), store on raw flash | `nucleo-f767zi-cmsis-lwip-norflash` | `SolidSyslogBddTargetCmsis` | `build-nucleo-cmsisrtos-lwipsocket-norflash` | `solid-syslog-nucleo-f767zi-cmsis-lwip-norflash` |
| [FreeRTOS-Plus-TCP](../../FreeRtos/README.md) | `nucleo-f767zi-plustcp` | `SolidSyslogBddTarget` | `build-nucleo-freertos-plustcp` | `solid-syslog-nucleo-f767zi-plustcp` |

All five build in the `cpputest-freertos-cross` image.
[`docs/builds.md`](../../../../docs/builds.md) gives each preset's commands and
where its ELF lands. The bring-up build leaves a raw `.bin` beside its ELF. The
two CMSIS-RTOS2 images are built over Mbed TLS. The wolfSSL build of that
target is not built for this board.

CI builds the BDD images with no addresses. An image that is to run on the
board is configured with those of the network under test:

```bash
cmake --preset nucleo-f767zi-lwip \
    -DSOLIDSYSLOG_BDD_BOARD_IP=<board address> \
    -DSOLIDSYSLOG_BDD_BOARD_NETMASK=<netmask> \
    -DSOLIDSYSLOG_BDD_BOARD_GATEWAY=<gateway address> \
    -DSOLIDSYSLOG_BDD_BOARD_ORACLE=<collector address>
```

What the rest of this page says of the BDD target applies to all four images
unless it names one.

## Network

The board takes a static address. Its address, netmask and gateway, and the
address of the collector, are the four cache variables above. They describe the
network under test, so none is written in this repository.
`SOLIDSYSLOG_BDD_BOARD_MAC` sets the hardware address as six hexadecimal octets
separated by colons, and defaults to a locally administered one.

Left unset, the addresses default to ones from the block RFC 5737 reserves for
documentation, so the image CI uploads reaches nothing. A collector on another
network is reached through the gateway.

The PHY negotiates its link and the target follows it. Each change of the link
is a line on the console:

```text
[board] link down
[board] link up, 100 Mbit/s full duplex
```

The lwIP images give the speed and duplex, say `link down` as they boot,
and poll the PHY four times a second. The FreeRTOS-Plus-TCP image says only
`[board] link up` and `[board] link down`, when its stack reports a change, and
polls twice a second while the link is up. It says nothing until the network
first comes up and starts its prompt only then, so booted without a cable it
shows its banner and no prompt.

A send while the link is down fails, and the library reports it as it reports
any failed delivery. The FreeRTOS-Plus-TCP image's datagram reports first that
the next hop is unresolved.

The target sets the PHY to auto-negotiate as it boots, and resets it before it
resets the MAC. The PHY takes its mode from strap pins as it leaves reset, and
after a reset through the debug probe about one boot in twenty latches another
mode. Most of those have negotiation off and would never report a negotiated
link. One powers the PHY down, which removes the MAC's 50 MHz reference clock,
and the MAC's reset completes only while that clock is present.

If the MAC's reset does not complete under lwIP, the console says
`[board] ethernet the MAC did not start` and the interface stays without a
link. Under FreeRTOS-Plus-TCP the stack retries the reset at intervals and says
nothing. The PHY is restarted only before the first attempt, so a MAC that
restart did not bring a clock to stays down until the board is reset.

Under lwIP the interface copies every frame between lwIP's memory and its own
buffers, which the Ethernet DMA reads and writes. FreeRTOS-Plus-TCP's interface
copies nothing: the DMA is handed the stack's own buffers, of which there are
sixteen. Both sets of buffers are kept out of the data cache, as described
under [Caches](#caches).

## Caches

Every image, the bring-up image included, runs with the instruction and data
caches on. The board support enables them before anything else.

The Ethernet DMA reads and writes memory without the processor, so the data
cache can hold a stale copy of that memory, and the DMA never sees what the
cache has not yet written back. The DMA's descriptors and frame buffers are
therefore kept in the first 32 KiB of RAM, which the board support marks as a
non-cacheable region in the memory protection unit. The linker script names the
region and each interface places its descriptors and buffers there.
FreeRTOS-Plus-TCP's sixteen buffers set its size.

The alternative is to leave that memory cached and clean or invalidate the
cache around each transfer. ST's Ethernet driver, compiled unmodified, reads
and writes the descriptors with no cache maintenance, so the descriptors need
the region whatever is done for the buffers. Putting the buffers there too
leaves one mechanism, set once at start-up. The cost is that frames are handled
in uncached memory.

With the caches on and the region not set, no message from the board reaches
the collector over UDP or TCP.

## Clock

`set time <seconds>` at the console gives the board UTC in seconds since the
epoch. The STM32's real-time clock keeps it from then on, on its own
32.768 kHz crystal, through every reset. Once set, the board sends a TIMESTAMP
on each message and `tzKnown="1" isSynced="1"`. Until then it sends NILVALUE
and `tzKnown="0" isSynced="0"`. Only the USB cable powers the clock, so a board
that has been unplugged has to be told the time again. A BDD run sets it
[before its first scenario](#running-the-bdd-scenarios).

The real-time clock supplies whole seconds and interrupts as each one starts.
The fraction is measured on the core's cycle counter, which counts the 216 MHz
system clock, so a reading is to the microsecond and costs a few instructions.
The real-time clock's own sub-second count resolves about four milliseconds.
The cycle counter belongs to the core's debug unit and runs with no debugger
attached. Using it leaves all the part's timers free.

The crystal and the system clock drift against each other by some tens of
microseconds a second. A reading never passes the end of its second, and the
next second starts where the real-time clock puts it, so the time never goes
back. After a reset, until the first interrupt, the fraction comes from the
real-time clock's sub-second count as read at start-up.

The real-time clock holds two digits of year and is taken as set when the year
is not zero, so `set time` accepts 2001 to 2099 and answers `set: invalid`
otherwise. It answers the same if the real-time clock could not be started:
either the crystal did not start, which the target waits up to five seconds for
when first powered, or other firmware run since power-up left the clock on
another source.

The bring-up image does not start the clock.

## Disk

The disk is a MicroSD card in a breakout on SPI4, wired as in the
[pin plan](#storage-breakouts-spi4). `set store file` mounts the target's
filesystem on it, formatting first if it finds none, and puts the target on a
file store there. The disk is the first 8 MiB of the card, the size the same
target has under QEMU. A smaller card is refused.

Two targets use the card, each with the filesystem it has under QEMU: FatFs
under the FreeRTOS + lwIP Raw target and FreeRTOS-Plus-FAT under the
FreeRTOS-Plus-TCP target. The CMSIS-RTOS2 target keeps its store on the
[flash](#w25q128-flash).

`set store wipe` leaves the card with no filesystem, so that the next
`set store file` formats it. It is refused once the store is on the disk. Wipe
a card in two cases:

- It has been used elsewhere. A card as sold, or as a computer formats it, has
  a partition table naming a filesystem that reaches beyond the 8 MiB. FatFs
  follows the table, so its mount either fails or succeeds on a filesystem the
  disk mostly cannot reach, and does not format. FreeRTOS-Plus-FAT has not been
  tried on such a card.
- The other image has used it. FatFs formats with no partition table and
  FreeRTOS-Plus-FAT with one, and neither formats over a filesystem it can
  mount: FreeRTOS-Plus-FAT mounted the filesystem FatFs left and took up the
  store it found there.

The run wipes the card before every scenario that uses a store and before a
sustained run through the file store, so this matters only for a store asked
for by hand at the console.

The card driver is this repository's own, in the targets' common code, and is
unit-tested on the host against a fake card. It drives the card in SPI mode, a
sector at a time. It supports a version 2 card of standard or high capacity,
and refuses a version 1 card or an MMC. It has been run on one card, a SanDisk
Ultra 8 GB microSDHC, so the standard-capacity path has been run against the
fake only.

The card is started at 211 kHz and run at 6.75 MHz. The breakout is on a
breadboard at the end of jumper wires, and that rate is half the fastest found
clean on them: 3000 messages went through the file store at 13.5 MHz with no
transfer repeated, and at 27 MHz the card did not come up. The card's CRC
checking is on, so a command or sector damaged on the wires is refused by the
end that receives it. A read or write that fails this way is retried twice, and
the target says so on the console, so failing wires show before they stop a
run. A sector that still does not transfer has the card restarted and is tried
once more, which recovers a card that lost its place when a wire came out and
went back.

Each byte is exchanged through SPI4's data register directly. ST's HAL call
times every transfer against the millisecond count, so a task switched out for
longer than the timeout is told that a completed byte failed. Under load that
showed as sectors failing their CRC and a store that froze. A card must be
started at no more than 400 kHz and SPI4's slowest rate is its bus clock
divided by 256, so the board runs that bus at 54 MHz where the part allows 108.

Any fault with the card is a console line beginning `[board] disk:`, with the
sector where there is one and the reason, such as no answer, timed out or CRC
failed. A missing card, or a wire out of the breadboard, gives
`[board] disk: the card did not start: no answer`. `set store file` then
answers `set: invalid` and leaves the target on the store it started with.

## W25Q128 flash

The CMSIS-RTOS2 target keeps its store on a W25Q128 flash in a breakout on
SPI4, wired as in the [pin plan](#storage-breakouts-spi4). There are two images
of it. One keeps the store in files [on LittleFS](#under-littlefs), and the
other keeps it on the part directly, [with no filesystem](#with-no-filesystem).
Each uses the first 8 MiB of the part's 16.

The target drives the part through the library's
[SPI NOR flash pack](../../../../docs/platforms/spinorflash/index.md), which
learns the part from its SFDP table, over the bus functions in
`BddTargetBoardSpiNorFlashBus.c`. The bring-up image has a flash driver of its
own instead, in `Bringup/`, with none of the library in it: that one drives a
Winbond W25Q of no more than 16 MiB, recognised by its JEDEC id, and refuses
any other. Both are unit-tested on the host against a fake part that programs
and erases as a real one does. The board's part is a W25Q128JV, which
identifies itself as `EF4018`.

The part runs at 6.75 MHz, as the [card](#disk) does on the same wires. Unlike
the card it sends no check with what it reads out, so damage on the wires would
go undetected. On this part a sector erase took 52 to 58 ms and a page program
about 1 ms. The task that asked for either polls the part's status until it
finishes, and the bus's `WhileBusy` yields between one ask and the next, so the
target's other thread runs meanwhile. It yields where it could sleep because
the kernel's tick is 10 ms, ten times a page program. A part that was erasing
when the board was reset carries on, and answers only its status until it
finishes, so the pack waits for that before asking it anything else.

Both images count what they ask of the part, so that what a store costs it can
be measured. `set store report` prints the counts as one line:

```text
[store] flash: erases 12, programs 340, reads 5120
```

Each is a count of calls at the pack's flash interface since the image started
the part, where a program is one call whatever it spans. A sustained run
through the store ends with what its messages cost in each. An image that keeps
its store on the card keeps no count, and answers `set: invalid`.

### Under LittleFS

`set store file` mounts LittleFS on the part, formatting first if it finds
none, and puts the target on a file store there. The filesystem is the size the
same target has under QEMU.

A filesystem block is one of the part's 4 KiB sectors, so LittleFS's erase is
the part's own, and a program clears bits and sets none. Under QEMU the same
target has a disk of sectors, where an erase is a write of `0xFF` and a program
can set bits.

LittleFS is given a program unit of 16 bytes and not the 256 of a page, for the
reason the
[LittleFS setup page](../../../../docs/platforms/littlefs/setup.md#what-will-catch-you-out)
gives, and the 512-byte cache it has under QEMU. The
[LittleFS platform page](../../../../docs/platforms/littlefs/index.md#what-the-store-costs-the-flash-in-erases)
says what a stored message costs in erases, and
[A store on NOR flash](../../../../docs/roles/flash-store-costs.md) has the
pace the store kept up with on this part.

`set store wipe` erases the two blocks that hold LittleFS's superblock, so that
the next `set store file` finds no filesystem and formats. It is refused once
the store is on the part. That `set store file` prints one line of LittleFS's
own before it answers, `error: Corrupted dir pair at {0x0, 0x1}`, which is its
mount finding the two blocks erased.

A fault the pack reports is two console lines. The pack reports the reason
through the library's error handler, as a line beginning `[solidsyslog]` that
names `SpiNorFlash` and ends with the reason, such as `NO_ANSWER`, `TIMED_OUT`
or `WRITE_REFUSED`. The target then says which operation met it, as a line
beginning `[board] flash:`, with the block where there is one. A missing part,
or a wire out of the breadboard, gives `NO_ANSWER` and then
`[board] flash: the part did not start`. The target says that line once for as
long as the part goes on failing to start. `set store file` then says
`[solidsyslog] littlefs mount failed`, answers `set: invalid` and leaves the
target on the store it started with.

### With no filesystem

`set store file` puts the target on a store whose blocks are the part's own
4 KiB sectors, the first 2048 of them, through the library's
`SolidSyslogNorFlashBlockDevice`. A store block is all a sector holds beyond
the four bytes that device keeps there for the block's state, unless the run
sets a `max-block-size`. The size a run sets is passed on as it stands, and the
device refuses one larger than a sector holds.

`set store wipe` marks each block that exists as disposed and erases nothing,
because the store erases every sector as it takes it. It is refused once the
store is on the part.

[A store on NOR flash](../../../../docs/roles/flash-store-costs.md) has the
pace the store kept up with and what it costs the part, beside the same for
LittleFS.

A fault the pack reports is the pack's own console line, as described above.
This image adds no `[board] flash:` line after it. Where the part does not
start, `set store file` then answers `set: invalid` and leaves the target on
the store it started with.

## What the board does not have yet

- TLS is linked, because the target is the same code as on QEMU, but the
  board's hardware random number generator is not wired in.
  [What has been run on hardware](../../../../docs/bdd.md#what-has-been-run-on-hardware)
  says what has been run on the board and what has not.

## Tools on the development machine

No IDE is needed.

| Tool | From | Used for |
|---|---|---|
| `STM32_Programmer_CLI` | STM32CubeCLT, or STM32CubeProgrammer alone | Flash and reset |
| `ST-LINK_gdbserver` | STM32CubeCLT | The gdb server |
| `arm-none-eabi-gdb` | STM32CubeCLT, or any Arm GNU toolchain | Debug |
| A serial terminal | Any | The console |
| `gh` | GitHub CLI | Fetching the image CI built |
| A syslog collector | The OpenTelemetry Collector under `Bdd/otel/` | Receiving what the board sends |
| Python, with `Bdd/requirements.txt` and `pyserial` | `pip install -r Bdd/requirements.txt pyserial` | Running the BDD scenarios |
| Docker | Any | Building the image the BDD run writes to the board |

STM32CubeCLT, ST's command-line toolset, carries the first three. On Windows
its installer puts them on `PATH` and offers the ST-LINK's USB driver, which
the board needs.

The board connects by the USB socket at the ST-LINK end, opposite the Ethernet
socket. That cable also powers it.

## Flash

The bring-up image is the one CI built for the commit. Fetch it, then write and
verify it:

```bash
gh run download <run-id> --name solid-syslog-nucleo-f767zi-bringup --dir nucleo
STM32_Programmer_CLI -c port=SWD -w nucleo/Bdd/Targets/Boards/NucleoF767zi/Bringup/SolidSyslogNucleoF767ziBringup.elf -v -rst
```

A BDD image is the one built for the network under test, written the same way
from where its build leaves it:

```bash
STM32_Programmer_CLI -c port=SWD -w build/nucleo-f767zi-lwip/Bdd/Targets/FreeRtosLwip/SolidSyslogBddTargetLwip.elf -v -rst
```

An ELF carries its own load address. The `.bin` does not, and goes at the start
of flash: `-w <file>.bin 0x08000000`.

## Reset

```bash
STM32_Programmer_CLI -c port=SWD -hardRst
```

The black button on the board, B2, does the same.

## Console

The ST-LINK presents USART3 as a virtual COM port: 115200 baud, 8 data bits, no
parity, 1 stop bit, no flow control. On Windows, Device Manager lists it as
*STMicroelectronics STLink Virtual COM Port*.

The bring-up image does not echo characters as they are typed. It answers a
whole line, ended by CR or LF, with `echo:` and the line. After `probe` it also
says what each storage breakout answered:

```text
probe: card CMD0 01 CMD8 01 000001AA - answered
probe: flash JEDEC id EF4018 - answered
```

A card answers its first command with `01`, and a flash gives an id that is not
all ones. A breakout that is missing or not wired reads as `FF` throughout.

After `flash` it erases the last sector of the flash, programs it three times
and erases it again, reading the whole sector back each time:

```text
flash: started, 16777216 bytes, checking sector 4095
flash: erased the sector in 54 ms, and it reads as erased
flash: a program across two pages, 300 bytes in 1 ms, and the sector reads back
flash: a program of part of a page, 16 bytes in 1 ms, and the sector reads back
flash: a second beside it, 16 bytes in 0 ms, and the sector reads back
flash: erased the sector in 56 ms, and it reads as erased
flash: passed
```

That sector is beyond the 8 MiB either image gives its store, so a store on the
part is not disturbed. A step that fails says which and why, and the last line is
`flash: failed`.

The BDD target echoes each character and shows a `SolidSyslog>` prompt a few
seconds after reset, once the network has had time to come up. `send 1` sends
one message, `set transport tcp` and `set transport udp` choose the transport,
and `set shutdown 1` ends the run: the console says `exit: ok` and the part
halts until it is reset. A run that ends in failure says `exit: failed` and its
exit status, and lights LD3, the red LED.

## Collector

Anything that listens on port 5514 for RFC 5424 over UDP, and over TCP with
RFC 6587 octet counting, at the address the image was built to reach. The
OpenTelemetry Collector under `Bdd/otel/` does, with the configuration that
listens on every interface:

```bash
VXWORKS64_COLLECTOR_PORT=5514 Bdd/otel/bin/otelcol-contrib --config Bdd/otel/config.vxworks64.yaml
```

It writes each message it receives to `Bdd/output/received_udp.jsonl` or
`Bdd/output/received_tcp.jsonl`, as a record of the fields its RFC 5424 parser
found.

## Running the BDD scenarios

With the board attached by USB and cabled to the test network, from Windows
PowerShell at the root of the repository:

```powershell
.\Bdd\Targets\Boards\NucleoF767zi\Run-NucleoF767ziBdd.ps1 -Target lwip -BoardAddress <board address> -CollectorAddress <this machine's address>
```

The script builds the image from the commit checked out, in the cross image CI
uses, writes it to the board, starts the collector and runs Behave.

| Parameter | Meaning |
|---|---|
| `-Target` | Required. `lwip` for the FreeRTOS + lwIP Raw image, `cmsis` for CMSIS-RTOS2 + lwIP Sockets, `cmsis-norflash` for the same with its store on raw flash, `plustcp` for FreeRTOS-Plus-TCP |
| `-Netmask`, `-Gateway` | The other two [network](#network) settings. Each of the four can be left in an environment variable, which the script's help names |
| `-SerialPort` | The console's port, where it is not to be found by ST's USB vendor id |
| `-SkipBuild` | Writes the image already built |
| `-Paths` | Runs only the features given |
| `-SustainedRun <count>` | Runs no scenario, and sends that many messages over UDP and then over TCP |
| `-FileStore` | With `-SustainedRun`, sends every message through a file store on the image's medium, which the run wipes first |
| `-FileStoreInterval <seconds>` | The pace of a file-store run, in place of the image's own |

The scenarios are those tagged `@udp` or `@tcp`, less what the board
[does not have yet](#what-the-board-does-not-have-yet). The run sets the
board's clock, so it runs the `@rtc` scenarios and not the `@no_rtc` ones. The
script holds the tag filter.

Behave drives the target as it does under QEMU, with two differences. The
console is the ST-LINK's virtual COM port, and a scenario starts the target by
resetting the board through the debug probe. A target's exit is read from the
console: `exit: ok`, or `exit: failed` and a status.

Before the first scenario the run reads the banner, and stops unless it names
the image asked for and the commit checked out. The build marks an image
`-dirty` when a tracked file has uncommitted changes, so the script refuses to
start from such a tree. The run then tells the board this machine's time, once.
The board's [clock](#clock) keeps it through the reset that starts each
scenario. A sustained run does the same before its first message.

Before a scenario tagged `@store` the run resets the board and has it wipe its
medium, so the scenario starts with no store on it, as it does under QEMU on a
new disk image.

Where a scenario kills the target to stand in for a power cut, the run asks the
target to shut down, as it does under QEMU. The target closes the store's files
and unmounts the filesystem before it exits. The scenario shows that a store
laid down by one run of the target is read by the next. It is not a loss of
power: the files are closed and the media stay powered, so it says nothing
about a write that a power cut would have interrupted.

A sustained run passes if the collector recorded every message once, in order,
and as the target sent it. Only the time, sequence number, up-time and library
version are not held to a value, and a message's time must be no earlier than
that of the one before. The run ends with a line for each transport giving the
messages sent, received and at fault. Through the file store each message is
written to the medium before it is sent, so the run paces itself at twenty a
second on the card and on the flash with no filesystem, and four a second on
LittleFS. Sent faster than the medium
takes them, messages fill the target's buffer and are discarded, which is the
library's discard policy at work.

When something goes wrong:

- A board that shows no banner within five seconds of a reset is reset again,
  up to three times. This has been seen once in about a thousand resets and its
  cause is not known. The run prints a line beginning
  `NUCLEO-F767ZI: the board showed no banner`, with what the board did show and
  what the probe reported. That line goes in the run's record.
- The programmer has failed a reset once in some thousands. It is asked again,
  up to three times and a second apart, because its failures come in bursts.
  The line begins `NUCLEO-F767ZI: the probe did not reset the board`, and goes
  in the record too.
- A target that stops says why first. A kernel assertion, a stack overrun, an
  exhausted kernel heap, an lwIP assertion, a FreeRTOS-Plus-TCP stack that will
  not start and a hard fault each give one line beginning `[board] fault:`,
  with the file and line, the task or the faulting address. The target then
  halts with the red LED lit.
- A step that times out waiting for the prompt sends the target an empty line
  and reports the answer, which separates a stopped target from a prompt that
  did not arrive.
- `Bdd/output/nucleo-f767zi-console.log` holds every byte that crossed the
  serial port in either direction, and every reset, each with its time.

## Attach gdb

Start the server on SWD (`-d`), attaching to the running part (`-g`), and tell
it where STM32CubeProgrammer's `bin` directory is (`-cp`):

```bash
ST-LINK_gdbserver -d -g -p 61234 -cp <STM32CubeProgrammer bin directory>
```

Then, from another terminal, with the ELF that is on the board:

```bash
arm-none-eabi-gdb SolidSyslogNucleoF767ziBringup.elf -ex "target remote localhost:61234"
```

The part halts where it was when gdb connected, so a backtrace shows what it
was doing. Without `-g` the server resets the part as gdb connects and halts it
at the reset vector, which is how to debug from the start. The server serves
one session and exits when gdb detaches.

## Pin plan

The pins the firmware uses, so that nothing wired to the board lands on one of
them.

### On the board

| Function | Pin | Notes |
|---|---|---|
| HSE clock in | PH0 | 8 MHz from the ST-LINK's MCO. No crystal is fitted, so HSE runs in bypass |
| LSE crystal | PC14, PC15 | 32.768 kHz, for the real-time clock |
| Console TX, USART3 | PD8 | To the ST-LINK virtual COM port |
| Console RX, USART3 | PD9 | From the ST-LINK virtual COM port |
| LD1, green | PB0 | The bring-up image blinks it |
| LD2, blue | PB7 | |
| LD3, red | PB14 | Lit, and left lit, if the clock or the console will not start, or a run ends in failure |
| B1, user button | PC13 | Unused |
| SWDIO, SWCLK | PA13, PA14 | The ST-LINK |
| SWO | PB3 | The ST-LINK; left free for trace |

### Ethernet, RMII

The PHY is the on-board LAN8742A, at address 0 on the management bus.

| Signal | Pin |
|---|---|
| REF_CLK | PA1 |
| MDIO | PA2 |
| CRS_DV | PA7 |
| MDC | PC1 |
| RXD0 | PC4 |
| RXD1 | PC5 |
| TX_EN | PG11 |
| TXD0 | PG13 |
| TXD1 | PB13 |

Jumpers JP6 and JP7 carry CRS_DV and TXD1 to the PHY, and both must be fitted
for Ethernet.

PA7 is also Arduino D11, the MOSI of the SPI on the Arduino header, so that SPI
cannot be used with Ethernet as the board ships. The breakouts are on a
different SPI.

### Storage breakouts, SPI4

Two breakouts share SPI4 on the CN9 Zio connector, each with its own chip
select: an Adafruit 4682 MicroSD breakout, the [disk](#disk), and an Adafruit
5643 W25Q128 breakout, the [flash](#w25q128-flash). None of these pins is
shared with Ethernet, the ST-LINK or an LED.

![The seven wires from CN8 and CN9 to the two breakouts on a breadboard](storage-wiring.svg)

The drawing shows the board with the ST-LINK end nearest, and CN8 and CN9 down
its right-hand edge. Power, ground, clock, MISO and MOSI each leave the board
once and reach both breakouts through a row of the breadboard.

| Signal | Pin | Zio | Connector pin | MicroSD pad | W25Q128 pad |
|---|---|---|---|---|---|
| 3V3 | - | - | CN8 pin 7 | `3V` | `VIN` |
| GND | - | - | CN9 pin 12 | `GND` | `GND` |
| SCK | PE2 | D56 | CN9 pin 14 | `CLK` | `SCK` |
| MicroSD chip select | PE4 | D57 | CN9 pin 16 | `CS` (`D3`) | - |
| MISO | PE5 | D58 | CN9 pin 18 | `SO` (`D0`) | `MISO` |
| MOSI | PE6 | D59 | CN9 pin 20 | `SI` (`CMD`) | `MOSI` |
| W25Q128 chip select | PE3 | D60 | CN9 pin 22 | - | `CS` |

The six CN9 pins are adjacent in the connector's inner column, the one nearer
the processor, starting at the sixth socket from the CN8 end.

The MicroSD breakout runs at 3.3 V only and has no regulator. CN8 pin 9, the
socket beside the 3V3 one, is 5 V, and 5 V on the breakout's `3V` pad damages
the card. The W25Q128 breakout's `3V3` pad is the output of its own regulator
and is left unconnected, as are the MicroSD breakout's `D1`, `DAT2` and `DET`.

PE2 also comes out on CN10 pin 25. Wire only one of the two.

The firmware holds high whichever chip select it is not using, and each
breakout pulls its own select up, so a breakout is off the bus from power-up.

A high select keeps a card off the bus only once the card is in SPI mode, which
its first command puts it in. Until then it treats MOSI as its own command line
whatever its select is doing, so an image that uses the flash sends the card
that one command first.

On a breadboard, a device that has stopped answering is most often a wire that
has come out. The bring-up image's `probe` says which of the two still answers,
and its `flash` whether the flash still erases and programs.

## Target checklist

Run by hand against the commit under review, each step recorded as pass, fail
or not run. The record names the commit and the id of the CI run the bring-up
artifact came from.

### The bring-up image

1. The `build-nucleo-bringup` lane passed on the commit, and its artifact is
   downloaded.
2. `STM32_Programmer_CLI` writes the ELF from that artifact and its verify
   passes.
3. After a reset the console shows the banner, and the commit in it is the
   one the lane built. On `main` that is the commit itself. On a pull request
   it is the merge of the commit under review into `main`, so the commit under
   review is its second parent:

   ```bash
   gh api repos/cososo-ltd/solid-syslog/commits/<banner commit> --jq '.parents[].sha'
   ```

4. A line typed on the console comes back as `echo:` and the line, and three
   lines sent together, with no pause between them, come back as three.
5. LD1, the green user LED, blinks, on for half a second and off for half a
   second, and LD3, the red user LED, is off. The three user LEDs are the row
   marked *User LEDs*. The power LED beside them and the ST-LINK's own LED by
   its USB socket are lit throughout and are not part of this step.
6. `arm-none-eabi-gdb` attaches through `ST-LINK_gdbserver` and a backtrace
   shows the part inside `main`.
7. Where the pull request changes the bring-up image or the
   [storage breakouts](#storage-breakouts-spi4): the line `probe` is answered
   with a card that answered `01` to CMD0 and `01 000001AA` to CMD8, and a
   flash that answered its JEDEC id, and the line `flash` ends in
   `flash: passed`.

### The BDD targets

One list, run once for each image the pull request can have changed. A change
to the board's own code, to the run script or to what the targets share is a
change to all of them.

| Image | Lane | Script argument | Store |
|---|---|---|---|
| FreeRTOS + lwIP Raw | `build-nucleo-freertos-lwipraw` | `-Target lwip` | FatFs on the MicroSD card |
| CMSIS-RTOS2 + lwIP Sockets | `build-nucleo-cmsisrtos-lwipsocket` | `-Target cmsis` | LittleFS on the W25Q128 flash |
| CMSIS-RTOS2 + lwIP Sockets, store on raw flash | `build-nucleo-cmsisrtos-lwipsocket-norflash` | `-Target cmsis-norflash` | The block store directly on the W25Q128 flash |
| FreeRTOS-Plus-TCP | `build-nucleo-freertos-plustcp` | `-Target plustcp` | FreeRTOS-Plus-FAT on the MicroSD card |

The board is cabled to the test network, with a MicroSD card in its breakout
and the flash breakout wired.
Steps 2 to 4 are one run of
[`Run-NucleoF767ziBdd.ps1`](#running-the-bdd-scenarios), which stops at the
first of them that fails.

1. The image's lane and the `tools-nucleo-f767zi-runner` lane passed on the
   commit.
2. The script builds the image from the commit under review, with the test
   network's addresses, and `STM32_Programmer_CLI` writes it and its verify
   passes.
3. The run starts its first scenario, which it does only once the banner the
   board shows after a reset names the image and the commit under review.
4. Every scenario the tag filter selects passes. The record gives the counts
   Behave ends with: the scenarios passed, failed, and skipped by the filter.
5. By hand, at the console: `set store wipe` answers `set store=wipe`,
   `set store file` answers `set store=file`, and a `send 1` after them
   arrives at the collector.
6. The script run again with `-SkipBuild -SustainedRun 10000` passes, and
   again with `-SkipBuild -SustainedRun 2000 -FileStore`. The record gives the
   two lines of each: the messages sent, received and at fault over UDP and
   over TCP.
7. By hand, where the pull request changes the [clock](#clock): the USB cable
   is taken out and put back, and a `send 1` at the console arrives at the
   collector with `-` for its TIMESTAMP and `tzKnown="0"`. After
   `set time <seconds>` a second arrives with that time and `tzKnown="1"`, and
   after the board's reset button is pressed, a third does too.
8. By hand, where the pull request changes the board's Ethernet set-up or a
   network interface: the network cable is taken out and the console shows
   `[board] link down`; put back, the console shows `[board] link up`, and a
   `send 1` after it arrives at the collector.

A scenario that fails is recorded with what was found: fixed in the pull
request, or raised as an issue and the scenario tagged `@nucleof767ziwip`
until it is closed.
