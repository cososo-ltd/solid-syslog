# Platforms

A platform is a set of adapters wrapping one upstream thing - a network stack, a
TLS library, a filesystem, an OS - behind the library's vtables. Compile the ones
your target needs; every unfilled role falls back to a Core Null object.

Read across a row for what a platform gives you, down a column for who provides a
capability. Capabilities are coarser than [roles](../roles/index.md): TLS is the
Stream role with a TLS backend, and time / host are plain callbacks rather than a
vtable.

## Platform × capability matrix

| Platform | Wraps | Network | TLS | At-rest crypto | Files | Buffer | OS primitives | Time & host |
|---|---|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| [Posix](posix/index.md) | POSIX / BSD sockets | ● | | | ● | ● | ● | ● |
| [Windows](windows/index.md) | Win32 / Winsock | ● | | | ● | | ● | ● |
| [FreeRTOS](freertos/index.md) | FreeRTOS kernel | | | | | | ● | |
| [CMSIS-RTOS2](cmsisrtos/index.md) | CMSIS-RTOS2 API | | | | | | ● | |
| [VxWorks 6.4](vxworks64/index.md) | VxWorks 6.4 kernel API | ● | | | | | ● | ● |
| [FreeRTOS-Plus-TCP](plustcp/index.md) | FreeRTOS-Plus-TCP | ● | | | | | | |
| [lwIP (Raw API)](lwipraw/index.md) | lwIP Raw API | ● | | | | | | |
| [lwIP (Sockets API)](lwipsocket/index.md) | lwIP Sockets API | ● | | | | | | |
| [OpenSSL](openssl/index.md) | OpenSSL ≥ 3.0 | | ● | ● | | | | |
| [Mbed TLS](mbedtls/index.md) | Mbed TLS | | ● | ● | | | | |
| [FatFs](fatfs/index.md) | ChaN FatFs | | | | ● | | | |
| [LittleFS](littlefs/index.md) | LittleFS | | | | ● | | | |
| [FreeRTOS-Plus-FAT](plusfat/index.md) | FreeRTOS-Plus-FAT | | | | ● | | | |
| [C11 atomics](stdatomic/index.md) | `<stdatomic.h>` | | | | | | ● | |

The at-rest-crypto column is the keyed policies (HMAC-SHA256, AES-256-GCM); the
unkeyed CRC-16 policy is Core.

OS primitives is what a kernel supplies: a mutex, an atomic counter, and the tick
count an uptime callback reports. Time & host is wall-clock time and host
identity, which is the clock, hostname, process-id and sleep callbacks. An RTOS
gives the first without the second, so a FreeRTOS or CMSIS-RTOS2 target still
needs a clock from its integrator.

The Buffer column is a platform-backed Buffer, which today means the POSIX
message queue. Core ships the Passthrough and Circular buffers, so an unmarked
row is not a gap.

Store and Structured Data are roles Core fills directly - they're under
[Roles](../roles/index.md), not here.

## Bring your own

No shipped platform for your target? Filling a role is implementing one vtable.
The [role pages](../roles/index.md) state each contract; [Porting](../porting.md)
is the full guide.

<!-- markdownlint-disable MD033 - the sticky is styled HTML (.postit-note in brand.css); md_in_html keeps its body as Markdown. -->

<div class="postit-note" markdown>
**Or let us do it.**

We build and support SolidSyslog platform adapters - your RTOS, network stack,
filesystem or crypto library - and the tests that prove them.
[Talk to us about it](https://www.cososo.co.uk/?service=solidsyslog#contact).
</div>

<!-- markdownlint-enable MD033 -->
