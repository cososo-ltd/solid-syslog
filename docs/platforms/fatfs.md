# FatFs

`Platform/FatFs/` wraps ChaN FatFs as the File layer
([FatFs documentation](http://elm-chan.org/fsw/ff/)). RTOS-agnostic — bare-metal,
FreeRTOS, Zephyr, NuttX.

Fills the [File](../api/structSolidSyslogFile.md) role — the primitive beneath a
BlockDevice.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogFatFsFile`](../api/SolidSyslogFatFsFile_8h.md) | file — `f_sync` after every write |

## Requirements

Your `ffconf.h`, a `diskio.c` media driver, and — if `FF_FS_REENTRANT=1` — an
`ffsystem.c`.
