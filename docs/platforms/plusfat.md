# FreeRTOS-Plus-FAT

`Platform/PlusFat/` wraps FreeRTOS-Plus-FAT as the File layer
([FreeRTOS-Plus-FAT documentation](https://www.freertos.org/Documentation/03-Libraries/05-FreeRTOS-labs/04-FreeRTOS-plus-FAT/01-FreeRTOS-plus-FAT)).

Fills the [File](../api/structSolidSyslogFile.md) role — the primitive beneath a
BlockDevice.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogPlusFatFile`](../api/SolidSyslogPlusFatFile_8h.md) | file — `ff_fflush` after every write |

## Requirements

FreeRTOS — Plus-FAT is FreeRTOS-coupled. Supply an `FF_Disk_t` media driver and
`FreeRTOSFATConfig.h`.

Full setup is [Integrating FreeRTOS-Plus-FAT](../integrating-plusfat.md).
