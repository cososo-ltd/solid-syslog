# NUCLEO-F767ZI board

ST's NUCLEO-F767ZI: an STM32F767ZI, which is a Cortex-M7 with on-board
Ethernet, and an ST-LINK that gives flash, reset, debug and a serial port over
one USB cable. The `nucleo-f767zi-*` presets select it, the BDD target ones with
`SOLIDSYSLOG_BDD_BOARD=NucleoF767zi`.

CI builds firmware for this board and never runs it. Running it is done on a
development machine with the board attached, and recorded against the
[target checklist](#target-checklist) below. For the BDD target that is
[one command](#running-the-bdd-scenarios).

Everything a target needs from the board is in this directory, behind the
board seam. [`../Board.cmake`](../Board.cmake) says what a board supplies to the
build, and the `BddTargetBoard*.h` headers in [`../../Common/`](../../Common/)
say what it supplies to the code.

## What is here

| Part of the seam | File | How this board fills it |
|---|---|---|
| Toolchain | `cmake/arm-none-eabi-cortex-m7.cmake` | `arm-none-eabi-gcc` for a Cortex-M7 with the hard-float ABI |
| Startup and vectors | - | ST's `startup_stm32f767xx.s` and `system_stm32f7xx.c`, compiled from the pinned tree as they stand |
| Linker script | `stm32f767zi.ld` | 2 MiB of flash at `0x08000000`, 384 KiB of RAM at `0x20020000`, the first 16 KiB of it for the Ethernet DMA |
| Clock, NVIC and disk facts | `BddTargetBoardConfig.h` | 216 MHz, four priority bits, a disk geometry nothing reads yet |
| Network addresses | `BddTargetBoardAddresses.h.in` | Written into the build tree by the configure, from cache variables |
| Caches, clock, console, LEDs, millisecond count | `Board.c`, `stm32f7xx_hal_conf.h` | Over ST's HAL. Both [caches](#caches) are on. The millisecond count is TIM6, so SysTick is the kernel's |
| Console, exit and fault | `BddTargetBoard.c` | USART3 through `Board.c`; an exit is a line on the console and a halt, and so is a fault |
| Disk | `BddTargetBoardDisk.c` | None fitted: it never becomes ready |
| Ethernet under lwIP | `Lwip/BddTargetBoardNetif.c`, `Lwip/EthernetPhy.c` | A netif over ST's HAL Ethernet driver, and the PHY's link read from its standard registers |

`Bringup/` is not part of the seam. It is an image of its own, with no
SolidSyslog, no kernel and no network stack in it, built straight on `Board.c`:
a banner on the serial port, each line received echoed back, and the green LED
blinking. It is what tells a fault in the board or the toolchain from one in
anything built on them.

ST's HAL driver and CMSIS device files are not in the repository. The cross
image pins them, and the build reads them from `STM32F7_HAL_DRIVER_PATH` and
`STM32F7_CMSIS_DEVICE_PATH`.

## Build

In the `cpputest-freertos-cross` image. The bring-up image:

```bash
cmake --preset nucleo-f767zi-bringup
cmake --build --preset nucleo-f767zi-bringup --target SolidSyslogNucleoF767ziBringup
```

It lands at
`build/nucleo-f767zi-bringup/Bdd/Targets/Boards/NucleoF767zi/Bringup/SolidSyslogNucleoF767ziBringup.elf`,
with a raw `.bin` beside it. The `build-nucleo-bringup` CI lane builds the same
thing on every pull request and uploads both as the
`solid-syslog-nucleo-f767zi-bringup` artifact.

The FreeRTOS + lwIP Raw BDD target, which is
[`../../FreeRtosLwip/`](../../FreeRtosLwip/README.md) built for this board:

```bash
cmake --preset nucleo-f767zi-lwip \
    -DSOLIDSYSLOG_BDD_BOARD_IP=<board address> \
    -DSOLIDSYSLOG_BDD_BOARD_NETMASK=<netmask> \
    -DSOLIDSYSLOG_BDD_BOARD_GATEWAY=<gateway address> \
    -DSOLIDSYSLOG_BDD_BOARD_ORACLE=<collector address>
cmake --build --preset nucleo-f767zi-lwip --target SolidSyslogBddTargetLwip
```

It lands at
`build/nucleo-f767zi-lwip/Bdd/Targets/FreeRtosLwip/SolidSyslogBddTargetLwip.elf`.
The `build-nucleo-freertos-lwipraw` CI lane builds it on every pull request,
without the four addresses, and uploads it as the
`solid-syslog-nucleo-f767zi-freertos-lwip` artifact.

## Network

The board takes a static address. Its own address, its netmask, its gateway
and the address it reaches the collector at are facts about the network it is
plugged into, so they are build-time settings, the four cache variables above,
and none of them is written in this repository. `SOLIDSYSLOG_BDD_BOARD_MAC`
sets the hardware address, six hexadecimal octets separated by colons, and
defaults to a locally administered one.

Left unset, the addresses default to ones from the block RFC 5737 reserves for
documentation. That is what CI builds, so the image CI uploads reaches nothing,
and a run on the board uses an image built for the network under test.

The collector does not have to be on the board's own network: one that is not
is reached through the gateway.

The on-board PHY negotiates its link, and the target follows it. The target
puts the PHY in that mode itself as it boots, and resets it into the mode
before it resets the MAC. The PHY takes its mode from strap pins as it leaves
reset, and after a reset through the debug probe about one boot in twenty has
it in some other: most have negotiation off, where it would never report a
negotiated link, and one powers it down, where it gives the MAC no clock and
the MAC does not start. Each change of the link is a line on the console, so a
pulled cable can be told from a fault:

```text
[board] link down
[board] link up, 100 Mbit/s full duplex
```

A send while the link is down fails, and the library reports it as it reports
any failed delivery.

The interface copies every frame, in both directions, between lwIP's memory and
buffers of its own that the Ethernet DMA reads and writes. Those buffers are
kept out of the data cache, as [Caches](#caches) has it.

Jumpers JP6 and JP7 carry two of the RMII signals to the PHY, and both must be
fitted.

The MAC's reset completes only while the PHY supplies its 50 MHz reference
clock. If it does not complete, the console says
`[board] ethernet the MAC did not start` and the interface stays without a link.

## Caches

Every image on the board runs with the Cortex-M7's instruction cache and data
cache on, the bring-up image included. The board support turns them on before
anything else.

The Ethernet DMA reads and writes memory without the processor, so what the
data cache holds of that memory can be out of date, and what it has not yet
written back the DMA never sees. The DMA's descriptors and frame buffers are
therefore kept in 16 KiB at the bottom of RAM that the board support gives to
the memory protection unit as a region the data cache does not hold. The
linker script names the region and the interface places its descriptors and
buffers there.

The other way to keep that memory coherent is to leave it cached and clean or
invalidate the cache around each transfer. A region was chosen over that for
two reasons. ST's Ethernet driver, which is compiled as it stands, reads and
writes the descriptors itself and maintains no cache, so the descriptors need
the region whatever is done for the buffers. And with the buffers beside them
there is one mechanism, set once at start-up, and no call that every transfer
has to remember. What it costs is that the one copy each frame takes is to or
from memory that is not cached.

With the caches on and that region not set, no message from the board reaches
the collector, over UDP or over TCP.

## What the board does not have yet

- No storage is fitted. `set store file` is refused and the target stays on the
  store it started with, which holds nothing.
- TLS is linked, because the target is the same code as on QEMU, and has not
  been run. The board's hardware random number generator is not wired in.

## Tools on the development machine

Nothing here needs an IDE.

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

STM32CubeCLT is ST's command-line toolset and carries the first three, so it
is the one install. On Windows its installer puts them on `PATH` and offers
the ST-LINK's USB driver, which the board needs.

The board connects by the USB socket on the ST-LINK end, the one opposite the
Ethernet socket. That one cable powers it.

## Flash

The bring-up image is the one CI built for the commit. Fetch it, then write
and verify it:

```bash
gh run download <run-id> --name solid-syslog-nucleo-f767zi-bringup --dir nucleo
STM32_Programmer_CLI -c port=SWD -w nucleo/Bdd/Targets/Boards/NucleoF767zi/Bringup/SolidSyslogNucleoF767ziBringup.elf -v -rst
```

The BDD target is the one built for the network under test:

```bash
STM32_Programmer_CLI -c port=SWD -w build/nucleo-f767zi-lwip/Bdd/Targets/FreeRtosLwip/SolidSyslogBddTargetLwip.elf -v -rst
```

An ELF carries its own load address. The `.bin` does not, and goes at the
start of flash: `-w <file>.bin 0x08000000`.

## Reset

```bash
STM32_Programmer_CLI -c port=SWD -hardRst
```

The black button on the board, B2, does the same.

## Console

The ST-LINK presents USART3 as a virtual COM port: 115200 baud, 8 data bits,
no parity, 1 stop bit, no flow control. On Windows the port is the one Device
Manager lists as *STMicroelectronics STLink Virtual COM Port*.

The bring-up image does not echo characters as they are typed. It answers a
whole line, ended by CR or LF, with `echo:` and the line.

The BDD target echoes each character and shows a `SolidSyslog>` prompt once
the network has been given its chance to come up, a few seconds after reset.
`send 1` sends one message, `set transport tcp` and `set transport udp` choose
how, and `set shutdown 1` ends the run: the console says `exit: ok` and the part
halts until it is reset. A run that ends in failure says `exit: failed` and
lights LD3, the red LED.

## Collector

Anything that listens for RFC 5424 on UDP and on TCP with RFC 6587 octet
counting, on port 5514, at the address the image was built to reach. The
OpenTelemetry Collector under `Bdd/otel/` does, with the configuration that
listens on every interface:

```bash
VXWORKS64_COLLECTOR_PORT=5514 Bdd/otel/bin/otelcol-contrib --config Bdd/otel/config.vxworks64.yaml
```

It writes what it receives to `Bdd/output/received_udp.jsonl` and
`Bdd/output/received_tcp.jsonl`, each message as a record with the fields its
RFC 5424 parser took from it.

## Running the BDD scenarios

With the board attached by USB and cabled to the test network, from Windows
PowerShell at the root of the repository:

```powershell
.\Bdd\Targets\Boards\NucleoF767zi\Run-NucleoF767ziBdd.ps1 -BoardAddress <board address> -CollectorAddress <this machine's address>
```

That builds the image from the commit checked out, in the cross image CI uses,
writes it to the board, starts the collector, and runs Behave over the
scenarios the board can run. `-Netmask` and `-Gateway` set the other two
[network](#network) settings, and each of the four can be left in an
environment variable instead, which the script's help names. `-SkipBuild`
writes the image already built, and `-Paths` runs only the features given.

`-SustainedRun <count>` runs no scenario. It sends that many messages over UDP
and then as many over TCP, one at a time, and passes if the collector recorded
every one of them once, in order, and the same as the first but for its
sequence number and up-time. It ends with a line for each transport giving the
messages sent, received and at fault.

Behave reaches the target as it reaches one under QEMU, with two differences.
Its console is the ST-LINK's virtual COM port, found by ST's USB vendor id
unless `-SerialPort` names it. And a scenario starts the target by resetting
the board through the ST-LINK's debug probe, where under QEMU it starts the
emulator.

Before the first scenario the run reads the banner and stops unless it names
the commit checked out, so a run cannot pass against an image of anything else.
The build marks an image `-dirty` when a tracked file has changes its commit
does not hold, so the script refuses to start from such a tree.

A board that shows no banner within five seconds of a reset is reset again, up
to three times, and the run says so on its output with what the board did show
and what the probe reported. A board silent after a reset has been seen once in
about a thousand resets and its cause is not known, so a line beginning
`NUCLEO-F767ZI: the board showed no banner` goes in the run's record. So does
one beginning `NUCLEO-F767ZI: the probe did not reset the board`: the
programmer itself has failed a reset once in some thousands, and it too is
asked again, up to three times and a second apart, with what it said. The
second is there because its failures come together: three attempts one after
another have all failed within a fifth of a second.

A target that stops says why before it does. A kernel assertion, a task that
overran its stack, a kernel heap with nothing left, an lwIP assertion and a
hard fault each end in one line beginning `[board] fault:`, with the file and
line, the task, or the address of the instruction, and then a halt with the
red LED lit. A step that times out waiting for the prompt sends the target an
empty line and reports what it showed in answer, so a target that had stopped
is told from one whose prompt did not arrive. The run also keeps
`Bdd/output/nucleo-f767zi-console.log`: every byte that crossed the serial
port in either direction and every reset, each with its time, taken at the
port and so before anything in the run could lose it.

The scenarios are the ones tagged `@udp` or `@tcp`, less what the board
[does not have yet](#what-the-board-does-not-have-yet) and what any target
without a real-time clock leaves out. The script holds the tag filter.

Where a scenario kills the target to stand in for a power cut, the board is
reset through the probe. That restarts the processor and loses what was in RAM.
It is not a loss of power at the storage media, which keep their supply
throughout, so it says nothing about a write that power would have cut short.

## Attach gdb

Start the server on SWD (`-d`), attaching to the part as it runs (`-g`), and
telling it where STM32CubeProgrammer's `bin` directory is (`-cp`):

```bash
ST-LINK_gdbserver -d -g -p 61234 -cp <STM32CubeProgrammer bin directory>
```

Then, from another terminal, with the ELF that is on the board:

```bash
arm-none-eabi-gdb SolidSyslogNucleoF767ziBringup.elf -ex "target remote localhost:61234"
```

The part halts where it was when gdb connected, so a backtrace shows what it
was doing. Without `-g` the server resets the part as gdb connects and halts
it at the reset vector, which is the way to debug from the start. The server
serves one session and exits when gdb detaches.

## Pin plan

The pins the firmware built here uses or will use, so that nothing wired to
the board lands on one of them.

### On the board

| Function | Pin | Notes |
|---|---|---|
| HSE clock in | PH0 | 8 MHz from the ST-LINK's MCO. No crystal is fitted, so HSE runs in bypass |
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

PA7 is also Arduino D11, the MOSI of the SPI on the Arduino header, so that
SPI cannot be used with Ethernet as the board ships. The breakouts go on a
different SPI.

### Storage breakouts, SPI4

Both breakouts share SPI4, on the CN9 Zio connector, with a chip select each.
None of these pins is shared with Ethernet, the ST-LINK or an LED.

| Signal | Pin | Zio | CN9 pin |
|---|---|---|---|
| SCK | PE2 | D56 | 14 |
| MISO | PE5 | D58 | 18 |
| MOSI | PE6 | D59 | 20 |
| MicroSD chip select | PE4 | D57 | 16 |
| W25Q128 chip select | PE3 | D60 | 22 |

PE2 also comes out on CN10 pin 25. Only one of the two is to be wired.

Both breakouts take 3V3 and GND from CN8.

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

### The FreeRTOS + lwIP Raw BDD target

The board is cabled to the test network. Steps 2 to 4 are one run of
[`Run-NucleoF767ziBdd.ps1`](#running-the-bdd-scenarios), which stops at the
first of them that fails.

1. The `build-nucleo-freertos-lwipraw` and `tools-nucleo-f767zi-runner` lanes
   passed on the commit.
2. The script builds the image from the commit under review, with the test
   network's addresses, and `STM32_Programmer_CLI` writes it and its verify
   passes.
3. The run starts its first scenario, which it does only once the banner the
   board shows after a reset names the commit under review.
4. Every scenario the tag filter selects passes. The record gives the counts
   Behave ends with: the scenarios passed, failed, and skipped by the filter.
5. By hand, at the console: `set store file` answers `set: invalid`, and a
   `send 1` after it still arrives at the collector.
6. The script run again with `-SkipBuild -SustainedRun 10000` passes. The
   record gives its two lines: the messages sent, received and at fault over
   UDP and over TCP.

A scenario that fails is recorded with what was found: fixed in the pull
request, or raised as an issue and the scenario tagged `@nucleof767ziwip`
until it is closed.
