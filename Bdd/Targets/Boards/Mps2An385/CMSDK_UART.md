# CMSDK APB UART - driver contract

The register contract behind `CmsdkUart.{h,c}`, the polled UART0 driver that
is the console of the cross BDD targets under QEMU.

It records what the hardware requires, as distinct from what QEMU tolerates,
and marks each place where the two may differ. Those are the places where a
test passes on QEMU and would fail on mps2-an385 silicon.

## 1. Registers

### 1.1 The peripheral

The peripheral is the `cmsdk_apb_uart` block from Arm's Cortex-M System Design
Kit (CMSDK), specified in the *Cortex-M System Design Kit Technical Reference
Manual* (ARM DDI 0479C/D), section 4.3 *APB UART*. The MPS2 AN385 FPGA image
has five of them, UART0 to UART4, in the CMSDK APB subsystem: see *Application
Note AN385: Cortex-M3 SMM on V2M-MPS2* (ARM DAI 0385D), section 3.7.

The silicon RTL and QEMU implement the same programmer's model. They differ in
timing, in side effects, and in what happens between the bus access and the
wire.

### 1.2 Base addresses on `mps2-an385`

From AN385 section 3.7 Table 3-4 ("APB Memory Map"), corroborated by QEMU's
`hw/arm/mps2.c` machine model:

| UART  | Base       | RX IRQ | TX IRQ | Combined-overrun IRQ |
|-------|------------|--------|--------|----------------------|
| UART0 | 0x40004000 | 0      | 1      | 12 (shared)          |
| UART1 | 0x40005000 | 2      | 3      | 12 (shared)          |
| UART2 | 0x40006000 | 4      | 5      | 12 (shared)          |
| UART3 | 0x40007000 | 18     | 19     | none                 |
| UART4 | 0x40009000 | 20     | 21     | none                 |

The board uses UART0, whose base address `BddTargetBoard.c` passes to
`CmsdkUart_Init`. The others are listed for reference.

### 1.3 Register map (offsets from base)

| Offset | Name                  | Access   | Reset    |
|--------|-----------------------|----------|----------|
| 0x000  | DATA                  | R/W      | 0        |
| 0x004  | STATE                 | mixed    | 0        |
| 0x008  | CTRL                  | R/W      | 0        |
| 0x00C  | INTSTATUS / INTCLEAR  | R/W1C    | 0        |
| 0x010  | BAUDDIV               | R/W      | 0        |
| 0xFD0-0xFFC | PID4..CID3       | RO       | fixed    |

### 1.4 Bit semantics

#### DATA (0x000) - R/W, 8 valid bits

- A write loads the transmit holding byte and sets `STATE.TXFULL`. The byte is
  sent only if `CTRL.TX_EN=1`. In QEMU, a write while `STATE.TXFULL` is already
  set also sets `STATE.TXOVERRUN` and replaces `txbuf` with the new value.
- A read returns the last received byte and clears `STATE.RXFULL`.

#### STATE (0x004)

| Bit | Name      | Direction  |
|-----|-----------|------------|
| 0   | TXFULL    | RO from software (HW sets/clears) |
| 1   | RXFULL    | RO from software (HW sets/clears) |
| 2   | TXOVERRUN | W1C        |
| 3   | RXOVERRUN | W1C        |

Software cannot clear `TXFULL` or `RXFULL` directly. `TXFULL` clears when the
hardware finishes transmitting and `RXFULL` when DATA is read. The overrun
bits are write-1-to-clear: write `STATE = 0x4` to clear TXOVERRUN, `0x8` for
RXOVERRUN, `0xC` for both.

#### CTRL (0x008)

| Bit | Name      |
|-----|-----------|
| 0   | TX_EN     |
| 1   | RX_EN     |
| 2   | TX_INTEN  |
| 3   | RX_INTEN  |
| 4   | TXO_INTEN |
| 5   | RXO_INTEN |
| 6   | HSTEST    (high-speed test mode, leave 0) |

In QEMU the writable mask is `0x7F`, and bits 7 and above are dropped
silently.

#### INTSTATUS / INTCLEAR (0x00C)

One address: a read returns INTSTATUS and a write acts as INTCLEAR (W1C).

| Bit | Name | Set by HW when                                               |
|-----|------|--------------------------------------------------------------|
| 0   | TX   | TXFULL goes from 1 to 0 and `CTRL.TX_INTEN=1`                |
| 1   | RX   | RXFULL goes from 0 to 1 and `CTRL.RX_INTEN=1`                |
| 2   | TXO  | `STATE.TXOVERRUN=1` and `CTRL.TXO_INTEN=1` (combinational)   |
| 3   | RXO  | `STATE.RXOVERRUN=1` and `CTRL.RXO_INTEN=1` (combinational)   |

The TX and RX interrupts are edge-triggered. From the Zephyr driver: *"In CMSDK
UART [TX] is an edge interrupt, firing on a state change of TX buffer from full
to empty."* This matters only to an interrupt-driven driver, and this one is
polled.

The overrun bits in INTSTATUS are not latched independently. QEMU recomputes
them as `intstatus_overrun = state_overrun & ctrl_overrun_inten`, so clearing
an overrun bit in STATE clears the matching bit in INTSTATUS too.

#### BAUDDIV (0x010) - R/W, 20 valid bits in QEMU (`& 0xFFFFF`)

Baud rate = PCLK / BAUDDIV. The least legal value is 16.

- QEMU ignores a value below 16: `uart_baudrate_ok()` returns false, so
  `uart_update_parameters()` does nothing. The UART still transmits and
  receives on the host pipe, because QEMU's transmit path depends only on
  `CTRL.TX_EN`.
- On silicon a value below 16 gives no valid serial framing. AN385 does not
  specify the failure mode. Arm's mbed-OS `serial_baud()` calls
  `error("serial_baud")` for a divisor below 16.

The driver fixes `BAUD_DIVISOR` at 16 and has no call to change it.

## 2. Polled transmit - `CmsdkUart_PutChar`

### 2.1 Register sequence

Once, in `CmsdkUart_Init`:

1. Write `BAUDDIV`, with a value of at least 16.
2. Write `CTRL` with `TX_EN | RX_EN`.

The Zephyr driver writes them in this order. From a clean reset, which QEMU
and silicon both give (CTRL=0, STATE=0, BAUDDIV=0), that order is sufficient.

For each character:

1. Wait while `STATE & TXFULL`.
2. Write the byte to `DATA`.

Nothing else is needed. A polled producer that always waits for `TXFULL` to
clear cannot cause TXOVERRUN, so there is no overrun handling and no INTCLEAR.

```c
while (TransmitterIsBusy())   /* (STATE & TX_FULL_BIT) != 0 */
{
    Yield();                  /* memoryAccess->sleep(1) */
}
WriteDataRegister(c);         /* memoryAccess->write32(base+DATA, c) */
```

Zephyr's `uart_cmsdk_apb_poll_out()` and mbed-OS use the same sequence.

### 2.2 What to poll before writing DATA

`STATE.TXFULL` (bit 0). The hardware sets it when DATA is written and clears
it when the byte has been shifted out or, in QEMU, when the chardev backend
has accepted it. Reading STATE has no side effect.

### 2.3 TXOVERRUN

`STATE.TXOVERRUN` is set when software writes DATA while `TXFULL=1`. In QEMU
the new byte replaces the one pending in `txbuf`, and the transmit already
scheduled sends the new one. Each overrun in QEMU therefore loses the byte
that was pending. DDI 0479D does not
say which byte silicon loses, so assume that a byte was lost and that its
identity is unspecified.

To clear it, write `0x4` to `STATE`. That clears `STATE.TXOVERRUN`, and the
next update clears `INTSTATUS.TXO`. Writing INTSTATUS with bit 2 set clears
`STATE.TXOVERRUN` as well.

TXOVERRUN can arise only if two writers reach the UART at once: see
[section 5](#5-concurrency).

### 2.4 BAUDDIV below 16

On QEMU the guest runs and its output still appears on `-serial stdio`. On
silicon there is no valid serial framing. The fixed divisor of 16 avoids the
difference.

### 2.5 DATA written before `CTRL.TX_EN` is set

On QEMU the byte does not reach the host. With `TX_EN=0`, `TXFULL` stays set
and the byte never drains, so the next DATA write sets TXOVERRUN. On silicon,
DDI 0479D leaves a write to a disabled transmitter implementation-specific.
`CmsdkUart_Init` sets `TX_EN` before any `PutChar`.

## 3. Polled receive - `CmsdkUart_GetChar`

### 3.1 Register sequence

Once, in `CmsdkUart_Init`: `BAUDDIV` of at least 16, and `CTRL.RX_EN = 1`.

For each character:

1. Wait while `STATE & RXFULL` is 0.
2. Read the byte from `DATA`.

`CmsdkUart_GetChar` blocks, sleeping between polls as `PutChar` does.

### 3.2 RXFULL

- The hardware sets it when a complete byte has been shifted in, which needs
  `CTRL.RX_EN=1`. In QEMU, `uart_can_receive()` returns 0 unless RX_EN is set,
  and characters that arrive with RX_EN clear are dropped.
- A read of DATA clears it. Software needs no separate clear.

### 3.3 RXOVERRUN

Set when a new byte arrives while `RXFULL=1`. In QEMU the new byte overwrites
`rxbuf`. Clear it by writing `0x8` to `STATE` or to `INTSTATUS`.

In QEMU, `uart_can_receive()` returns 0 while RXFULL is set, so the chardev
backend stops feeding bytes until the guest reads DATA, and RXOVERRUN is hard
to provoke. Silicon has no such backpressure: a consumer that is too slow
loses bytes. The driver does not check for RXOVERRUN.

## 4. Reference implementations

In order of usefulness for checking the contract.

### 4.1 Zephyr `drivers/serial/uart_cmsdk_apb.c`

The most directly comparable. Linaro 2016 (modernised 2021), Apache 2.0. It
has the `while (uart->state & UART_TX_BF) {} uart->data = c` poll-out sequence
and a comment on the edge-triggered TX interrupt.
URL: `github.com/zephyrproject-rtos/zephyr/blob/main/drivers/serial/uart_cmsdk_apb.c`.

### 4.2 mbed-OS `targets/TARGET_ARM_FM/TARGET_FVP_MPS2/serial_api.c`

Arm's own HAL driver, Apache 2.0. It confirms the BAUDDIV minimum of 16, the
disable-before-reconfigure sequence and the W1C use of INTCLEAR.
URL: `github.com/ARMmbed/mbed-os/blob/master/targets/TARGET_ARM_FM/TARGET_FVP_MPS2/serial_api.c`.

### 4.3 QEMU device model - `hw/char/cmsdk-apb-uart.c`

Peter Maydell at Linaro, 2017, GPLv2. This is the authority on what QEMU
does, and the file to read when a test passes on QEMU and silicon may differ.
Its header comment gives DDI 0479C as the specification.
URL: `github.com/qemu/qemu/blob/master/hw/char/cmsdk-apb-uart.c`.

### 4.4 FreeRTOS demo `CORTEX_MPU_M3_MPS2_QEMU_GCC`

Not useful here. The upstream FreeRTOS demo for this board uses semihosting
(`BKPT 0xAB`) and not the CMSDK UART.

## 5. Concurrency

### 5.1 Writers must be serialised

The UART has a single holding register and no FIFO. The transmit critical
section is "wait while `STATE.TXFULL`, then write DATA". Two tasks that share
the UART without a lock interleave their output byte by byte, and can cause
TXOVERRUN.

The driver holds no lock. The targets' `_write` in
`Bdd/Targets/Common/Syscalls.c` holds the console lock around each write, and
that is what keeps one write whole.

### 5.2 The wait and the tick rate

At 115200 8N1 one character takes about 87 µs. The driver does not spin while
it waits: it calls the `sleep(milliseconds)` function it was given, with 1 ms.
The board passes `BddTargetOsPrimitives_Sleep`, which rounds that up to one
tick, 10 ms at the targets' `configTICK_RATE_HZ` of 100, so the task blocks
and others run.

On QEMU the transmit wait does not iterate while the chardev backend accepts
each byte as it is written. The host fake counts the calls
(`CmsdkUartFake_SleepCallCount()`), which lets the unit tests cover the wait
without threads.

### 5.3 What QEMU will not catch

| Trap                                       | QEMU              | Silicon           | Driver |
|--------------------------------------------|-------------------|-------------------|--------|
| BAUDDIV of 0, or below 16                  | Output works      | No output         | Divisor fixed at 16 |
| DATA written before TX_EN is set           | Byte lost silently| Implementation-defined | `Init` sets TX_EN first |
| Wait on TXFULL                             | Iterates only if the backend refuses a byte | About 87 µs a character at 115200 | Polls before each write, sleeping |
| RXOVERRUN with a slow consumer             | Hidden by chardev backpressure | Bytes lost | Not checked |

## References

- Arm DDI 0479C/D, *Cortex-M System Design Kit Technical Reference Manual*, section 4.3 *APB UART*: the programmer's model, register layout and reset values.
- Arm DAI 0385D, *Application Note AN385: Cortex-M3 SMM on V2M-MPS2*, section 3.7 *CMSDK APB subsystem*: the UART base addresses on `mps2-an385`.
- QEMU `hw/char/cmsdk-apb-uart.c`: the modelled semantics (W1C handling, edge-triggered TX, the chardev backpressure that hides RXOVERRUN, BAUDDIV below 16 ignored).
- QEMU `hw/arm/mps2.c`: the UART base address and IRQ table for the AN385 machine.
- Zephyr `drivers/serial/uart_cmsdk_apb.c`: the poll-out sequence and the edge-triggered TX comment.
- mbed-OS `targets/TARGET_ARM_FM/TARGET_FVP_MPS2/serial_api.c`: Arm's HAL, which corroborates the BAUDDIV minimum of 16.
