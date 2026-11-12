# NUCLEO-F767ZI target

Firmware for ST's NUCLEO-F767ZI board: an STM32F767ZI, which is a Cortex-M7
with on-board Ethernet, and an ST-LINK that gives flash, reset, debug and a
serial port over one USB cable.

CI builds this firmware and never runs it. Running it is done by hand, on a
development machine with the board attached, and recorded against the
[target checklist](#target-checklist) below.

## What is here

| Directory | Holds |
|---|---|
| `Board/` | The board support: the linker script, the HAL configuration, and the clock, console, LEDs and millisecond count over ST's HAL |
| `Bringup/` | The bring-up image, which carries no SolidSyslog. It prints a banner on the serial port, echoes each line it receives, and blinks the green LED |
| `cmake/` | The Cortex-M7 toolchain file |

ST's HAL driver and CMSIS device files are not in the repository. The cross
image pins them, and the build reads them from `STM32F7_HAL_DRIVER_PATH` and
`STM32F7_CMSIS_DEVICE_PATH`. The startup code and `system_stm32f7xx.c` are
ST's, compiled from that tree as they stand.

## Build

In the `cpputest-freertos-cross` image:

```bash
cmake --preset nucleo-f767zi-bringup
cmake --build --preset nucleo-f767zi-bringup --target SolidSyslogNucleoF767ziBringup
```

The image lands at
`build/nucleo-f767zi-bringup/Bdd/Targets/NucleoF767zi/SolidSyslogNucleoF767ziBringup.elf`,
with a raw `.bin` beside it. The `build-nucleo-bringup` CI lane builds the same
thing on every pull request and uploads both as the
`solid-syslog-nucleo-f767zi-bringup` artifact.

## Tools on the development machine

Nothing here needs an IDE.

| Tool | From | Used for |
|---|---|---|
| `STM32_Programmer_CLI` | STM32CubeCLT, or STM32CubeProgrammer alone | Flash and reset |
| `ST-LINK_gdbserver` | STM32CubeCLT | The gdb server |
| `arm-none-eabi-gdb` | STM32CubeCLT, or any Arm GNU toolchain | Debug |
| A serial terminal | Any | The console |
| `gh` | GitHub CLI | Fetching the image CI built |

STM32CubeCLT is ST's command-line toolset and carries the first three, so it
is the one install. Put its tools on `PATH`. The ST-LINK also needs its USB
driver on Windows, which the same installer offers.

The board connects by the USB socket on the ST-LINK end, the one opposite the
Ethernet socket. That one cable powers it.

## Flash

Fetch the image CI built for the commit, then write and verify it:

```bash
gh run download <run-id> --name solid-syslog-nucleo-f767zi-bringup --dir nucleo
STM32_Programmer_CLI -c port=SWD -w nucleo/Bdd/Targets/NucleoF767zi/SolidSyslogNucleoF767ziBringup.elf -v -rst
```

The ELF carries its own load address. The `.bin` does not, and goes at the
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

The firmware does not echo characters as they are typed. It answers a whole
line, ended by CR or LF, with `echo:` and the line.

## Attach gdb

Start the server on SWD (`-d`), telling it where STM32CubeProgrammer's `bin`
directory is (`-cp`):

```bash
ST-LINK_gdbserver -d -p 61234 -cp <STM32CubeProgrammer bin directory>
```

Then, from another terminal, with the ELF that is on the board:

```bash
arm-none-eabi-gdb SolidSyslogNucleoF767ziBringup.elf -ex "target remote localhost:61234"
```

The server halts the part on attach. `monitor reset` restarts it from the
reset vector, still halted.

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
| LD3, red | PB14 | Lit, and left lit, if the clock or the console will not start |
| B1, user button | PC13 | Unused |
| SWDIO, SWCLK | PA13, PA14 | The ST-LINK |
| SWO | PB3 | The ST-LINK; left free for trace |

### Ethernet, RMII

The PHY is the on-board LAN8742A.

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

PA7 is also Arduino D11, the MOSI of the SPI on the Arduino header, so that
SPI cannot be used with Ethernet as the board ships. The breakouts go on a
different SPI.

### Storage breakouts, SPI4

Both breakouts share SPI4, on the CN9 Zio connector, with a chip select each.
None of these pins is shared with Ethernet, the ST-LINK or an LED.

| Signal | Pin | Zio |
|---|---|---|
| SCK | PE2 | D56 |
| MISO | PE5 | D58 |
| MOSI | PE6 | D59 |
| MicroSD chip select | PE4 | D57 |
| W25Q128 chip select | PE3 | D60 |

Both breakouts take 3V3 and GND from CN8.

## Target checklist

Run by hand against the commit under review, each step recorded as pass, fail
or not run.

1. The `build-nucleo-bringup` lane passed on the commit, and its artifact is
   downloaded.
2. `STM32_Programmer_CLI` writes the ELF from that artifact and its verify
   passes.
3. After a reset the console shows the banner, and the commit in it is the
   commit under review.
4. A line typed on the console comes back as `echo:` and the line.
5. The green LED blinks, on for half a second and off for half a second, and
   the red LED is off.
6. `arm-none-eabi-gdb` attaches through `ST-LINK_gdbserver` and a backtrace
   shows the part inside `main`.
