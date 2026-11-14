# QEMU mps2-an385 board

The board the three cross BDD targets run on in CI: QEMU's `mps2-an385`
machine, a Cortex-M3 with CMSDK UARTs and a LAN9118 Ethernet controller. The
`freertos-cross`, `freertos-cross-lwip`, `cmsis-cross-lwip` and
`cmsis-cross-lwip-wolfssl` presets select it with
`SOLIDSYSLOG_BDD_BOARD=Mps2An385`.

Everything a target needs from the machine is in this directory, behind the
board seam. [`../Board.cmake`](../Board.cmake) says what a board supplies to the
build, and the `BddTargetBoard*.h` headers in [`../../Common/`](../../Common/)
say what it supplies to the code.

## What is here

| Part of the seam | File | How this machine fills it |
|---|---|---|
| Toolchain | `cmake/arm-none-eabi.cmake` | `arm-none-eabi-gcc` for a Cortex-M3 |
| Startup and vectors | `Startup.c` | Reset handler and a vector table with the 32 external interrupts; IRQ 13 is the Ethernet controller |
| Linker script | `mps2-an385.ld` | 4 MiB of flash at `0x00000000`, 4 MiB of RAM at `0x20000000` |
| Clock, NVIC, network and disk facts | `BddTargetBoardConfig.h` | 25 MHz, three priority bits, the slirp addresses, an 8 MiB disk |
| CMSIS device header | `BddTargetCmsisDevice.h` | The core's exceptions over `core_cm3.h` |
| Console, exit and fault | `BddTargetBoard.c` | CMSDK UART0 through `CmsdkUart.c`, polled; exit through ARM semihosting, which a fault takes too, after a line saying what it was |
| Disk | `BddTargetBoardDisk.c` | A flat image on the machine QEMU runs on, over semihosting |
| Ethernet under lwIP | `Lwip/BddTargetBoardNetif.c` | A netif over the vendored Arm controller driver in `Lwip/smsc9220/` |
| Ethernet under FreeRTOS-Plus-TCP | `PlusTcp/BddTargetBoardPlusTcp.c` | Upstream's `MPS2_AN385` network interface, with the interrupt priority it leaves unset |

## Console

`CmsdkUart.c` is a polled driver for CMSDK UART0 at `0x40004000`, which QEMU
surfaces over `-serial stdio`. It sleeps for a millisecond between polls of a
busy transmitter or an empty receiver, so the other threads run while the
console waits. [`CMSDK_UART.md`](CMSDK_UART.md) is the driver's contract, and
`Tests/FreeRtos/CmsdkUartTest.cpp` tests it on the host against a fake register
block.

## Disk

The machine has no storage, so the disk is `solidsyslog-disk.img` in QEMU's
working directory, reached through the semihosting trap. Behave removes it
around each scenario. A missing or short image is created as a sparse 8 MiB
file, which reads as zeros, so the filesystem above formats on first use. The
image survives a QEMU restart, which is what the power-cycle scenarios rely on.

A production port has real media under its filesystem. This is BDD-target
glue, not a reference driver.

## Network

QEMU's user-mode network, slirp. The targets take a static `10.0.2.15`; the
gateway `10.0.2.2` is routed to the machine QEMU runs on, where the oracle
listens, and `10.0.2.3` is slirp's DNS forwarder.

Under lwIP the controller driver is Arm's, vendored from FreeRTOS-Plus-TCP's
`MPS2_AN385` network interface (Apache-2.0; copyright and licence headers
preserved). It sits in `Lwip/smsc9220/` with a `DisableFormat` `.clang-format`
so the format check leaves third-party source alone. The snapshot carries
three known defects that are benign on QEMU and would matter on silicon:
[`Lwip/smsc9220/KNOWN-LIMITATIONS.md`](Lwip/smsc9220/KNOWN-LIMITATIONS.md) has
the provenance and the detail.

## Running an image

Build any of the targets with its preset, then give QEMU the ELF:

```bash
qemu-system-arm \
    -M mps2-an385 -m 16M \
    -display none -serial mon:stdio \
    -icount shift=auto,sleep=off,align=off \
    -netdev user,id=net0 \
    -net nic,netdev=net0,model=lan9118 \
    -semihosting-config enable=on,target=native \
    -kernel build/freertos-cross/Bdd/Targets/FreeRtos/SolidSyslogBddTarget.elf
```

The `SolidSyslog>` prompt appears once the network is up. `send 1` emits one
RFC 5424 message to the oracle's address. `quit` tears the target down and
leaves the scheduler idling, so end QEMU with `Ctrl+A` then `x`; `set shutdown
1` tears down and ends QEMU itself, through the semihosting exit.

`-serial mon:stdio` is what gives the terminal that `Ctrl+A` escape. The
semihosting option is what the disk and the exit go through; without it the
first use of either stops QEMU with a fault.

## Attaching GDB

Start QEMU paused, listening on `:1234`, by adding `-s -S` to the command
above. Then, in a second terminal:

```bash
arm-none-eabi-gdb \
    build/freertos-cross/Bdd/Targets/FreeRtos/SolidSyslogBddTarget.elf
(gdb) target remote :1234
(gdb) break main
(gdb) continue
```

## How it boots

The machine boots from `0x00000000`. The linker script places the vector table
at the start of the image, which QEMU loads at that address. `Reset_Handler`
in `Startup.c` initialises `.data` and `.bss`, calls `__libc_init_array`, then
calls `main()`.

The vector table names the kernel's handlers by their CMSIS names
(`SVC_Handler`, `PendSV_Handler`, `SysTick_Handler`). Each target's
`FreeRTOSConfig.h` maps the FreeRTOS port's own names onto them.
