# FreeRTOS

`Platform/FreeRtos/` wraps FreeRTOS kernel primitives
([FreeRTOS documentation](https://www.freertos.org/Documentation/00-Overview)).
Networking is a separate backend — [FreeRTOS-Plus-TCP](plustcp.md) or
[lwIP](lwip.md).

Fills the Mutex [role](../roles/index.md), plus a sysUpTime callback.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogFreeRtosMutex`](../api/SolidSyslogFreeRtosMutex_8h.md) | mutex (`xSemaphoreCreateMutexStatic`) |
| [`SolidSyslogFreeRtosSysUpTime`](../api/SolidSyslogFreeRtosSysUpTime_8h.md) | uptime (`xTaskGetTickCount`) |

## Requirements

`configSUPPORT_STATIC_ALLOCATION=1` — the mutex uses static allocation.
