# Platforms

A platform is a set of adapters wrapping one upstream thing — a network stack, a
TLS library, a filesystem, an OS — behind the library's vtables. Compile the ones
your target needs; every unfilled role falls back to a Core Null object.

Read across a row for what a platform gives you, down a column for who provides a
capability. Capabilities are coarser than [roles](../roles/index.md): TLS is the
Stream role with a TLS backend, and time / host are plain callbacks rather than a
vtable.

## Platform × capability matrix

| Platform | Wraps | Network | TLS | At-rest crypto | Files | OS primitives | Time & host |
|---|---|:-:|:-:|:-:|:-:|:-:|:-:|
| [Posix](posix.md) | POSIX / BSD sockets | ● | | | ● | ● | ● |
| [Windows](windows.md) | Win32 / Winsock | ● | | | ● | ● | ● |
| [FreeRTOS](freertos.md) | FreeRTOS kernel | | | | | ● | ● |
| [FreeRTOS-Plus-TCP](plustcp.md) | FreeRTOS-Plus-TCP | ● | | | | | |
| [lwIP (Raw API)](lwip.md) | lwIP Raw API | ● | | | | | |
| [OpenSSL](openssl.md) | OpenSSL ≥ 3.0 | | ● | ● | | | |
| [Mbed TLS](mbedtls.md) | Mbed TLS | | ● | ● | | | |
| [FatFs](fatfs.md) | ChaN FatFs | | | | ● | | |
| [FreeRTOS-Plus-FAT](plusfat.md) | FreeRTOS-Plus-FAT | | | | ● | | |
| [C11 atomics](atomics.md) | `<stdatomic.h>` | | | | | ● | |

The at-rest-crypto column is the keyed policies (HMAC-SHA256, AES-256-GCM); the
unkeyed CRC-16 policy is Core. Buffer, Store, and Structured Data are roles Core
fills directly (with a Posix message-queue buffer option) — they're under
[Roles](../roles/index.md), not here.

## Bring your own

No shipped platform for your target? Filling a role is implementing one vtable.
The [role pages](../roles/index.md) state each contract; [Porting](../porting.md)
is the full guide.

<!-- markdownlint-disable MD033 — the sticky is styled HTML (.postit-note in brand.css); md_in_html keeps its body as Markdown. -->

<div class="postit-note" markdown>
**Or let us do it.**

We build and support SolidSyslog platform adapters — your RTOS, network stack,
filesystem or crypto library — and the tests that prove them.
[Talk to us about it](https://www.cososo.co.uk/?service=solidsyslog#contact).
</div>

<!-- markdownlint-enable MD033 -->
