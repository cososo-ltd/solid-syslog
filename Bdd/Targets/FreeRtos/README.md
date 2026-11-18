# FreeRTOS BDD target

`SolidSyslogBddTarget` is the BDD target for the `FreeRtos`, `PlusTcp` and
`PlusFat` packs, with `MbedTls` for TLS. `SOLIDSYSLOG_BDD_TARGET=FREERTOS_PLUSTCP`
selects it and the `freertos-cross` preset builds it. In CI it runs under
`qemu-system-arm`, and the BDD harness drives it through the board's console.

What belongs to the board it runs on is behind the board seam: startup,
console, disk, Ethernet controller and addresses. The board CI uses is
described in [`../Boards/Mps2An385/`](../Boards/Mps2An385/README.md), which
also has the QEMU command, the GDB attach and how the image boots.

## What it links

- A `SolidSyslogCircularBuffer` guarded by a `SolidSyslogFreeRtosMutex`,
  drained by a Service task.
- A `SolidSyslogSwitchingSender` over three senders: UDP over
  `SolidSyslogPlusTcpDatagram`, TCP over `SolidSyslogPlusTcpTcpStream` and
  `SolidSyslogStreamSender`, and TLS over `SolidSyslogMbedTlsStream` on a
  second `SolidSyslogPlusTcpTcpStream`. TLS and mutual TLS share the third
  sender and differ in the port they connect to.
- A `SolidSyslogPlusTcpResolver`, given the address at which the board reaches
  the oracle.
- A `SolidSyslogNullStore` until `set store file` replaces it with a
  `SolidSyslogBlockStore` over `SolidSyslogPlusFatFile` on the board's disk.

The newlib syscalls, the shared pipeline, the console commands and each
filesystem's media glue are in [`../Common/`](../Common/), shared with the
other cross BDD targets. The disk glue is for these targets only and is not a
reference port: a product supplies its own media driver.

## Console commands

Behave drives the target over its console, as
[`Bdd/README.md`](../../README.md) describes.

| Command | Effect |
|---|---|
| `send N` | Sends N RFC 5424 messages over the selected transport |
| `set NAME VALUE` | Sets an identity, endpoint or PRIVAL field |
| `switch udp`, `switch tcp`, `switch tls`, `switch mtls` | Selects the transport. UDP is the default |
| `set store file` | Puts the target on the file store |
| `set max-blocks`, `set max-block-size`, `set discard-policy`, `set halt-exit` | Configure the block store, before `set store file` |
| `set no-reporting 1` | Turns off the logger's reports of its own events |
| `blocks` | Lists the blocks the store's block device holds as one `BLOCKS: 00 01` line, since Behave cannot see the disk image |
| `set shutdown 1` | Destroys the library's objects, which closes the store's files, unmounts the volume and exits through the board |
| `quit` | Tears the target down without exiting |

The store's disk outlives a restart of the target, which is what the
power-cycle replay scenarios rely on. The BDD step `the client is killed` uses
`set shutdown 1` on this target, so the next run's mount finds the directory
entries up to date.

## Tunables

[`solidsyslog_user_tunables.h`](solidsyslog_user_tunables.h) is this target's
tunables file, which the `freertos-cross` preset names in
`SOLIDSYSLOG_USER_TUNABLES_FILE`. It raises `SOLIDSYSLOG_MUTEX_POOL_SIZE` to 2,
because the target creates two mutexes where the library provides for one, and
takes every other tunable at the library default.
[Build integration](../../../docs/build-integration.md#tunables) describes the
mechanism.

## Build and run

[`docs/builds.md`](../../../docs/builds.md) has the build commands, and
[`docs/containers.md`](../../../docs/containers.md) the `freertos-target`
devcontainer service with its VS Code build, run and debug tasks.
[`Bdd/README.md`](../../README.md#freertos-on-qemu) has the Behave run.

## On the NUCLEO-F767ZI

The `nucleo-f767zi-plustcp` preset builds this target for ST's NUCLEO-F767ZI.
Nothing in this directory changes for it. The board supplies its own half of
the seam, the kernel configuration takes the core clock and the interrupt
priority bits from the board, and the stack's configuration takes from the
board whether its network interface copies frames. The interface there is the
one FreeRTOS-Plus-TCP ships for the STM32 MACs, and the disk is a MicroSD card.
CI builds the image and never runs it. The board's
[README](../Boards/NucleoF767zi/README.md) has the build, the run and the
target checklist.

## How it works

`main()` brings the board up, which readies the console for the polled `_read`
and `_write` in [`Syscalls.c`](../Common/Syscalls.c). Both tasks write that one
console, so `_write` holds a console lock, and stdio belongs to the interactive
task. The Service task's reports and markers each leave in one `write()`, so
the host sees every prompt and every report whole.

`main()` then starts FreeRTOS-Plus-TCP with a static IPv4 address on the
board's network, and starts the scheduler. The IP task's event hook reports
each change of the network on the console, as `[board] link up` or
`[board] link down`. On the first link-up it creates the interactive task,
which runs `BddTargetInteractive_Run`, and the Service task, which drains the
buffer into the switching sender.
