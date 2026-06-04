/* Integrator FreeRTOS-Plus-FAT configuration for the FreeRTOS-Plus-TCP BDD
 * target (QEMU mps2-an385). Plus-FAT is header-configured: ff_headers.h includes
 * this file (after FreeRTOS.h) and FreeRTOSFATConfigDefaults.h fills in every
 * value not set here. Resolved off the -I path (ff_headers.h's `#include
 * "FreeRTOSFATConfig.h"` finds no copy beside itself), so no source staging is
 * needed — unlike ChaN-FatFs's ffconf.h. Introduced in SolidSyslog S29.05.
 *
 * Mirrors the host-test config in Tests/Support/PlusFatFakes/Interface so the
 * adapter sees the same Plus-FAT layout on-target as under host TDD. The
 * matching configNUM_THREAD_LOCAL_STORAGE_POINTERS (>= 3) lives in
 * FreeRTOSConfig.h, which FreeRTOS.h pulls in before this file. */
#ifndef FREERTOSFATCONFIG_H
#define FREERTOSFATCONFIG_H

/* QEMU mps2-an385 (Cortex-M3) is little-endian. */
#define ffconfigBYTE_ORDER (pdFREERTOS_LITTLE_ENDIAN)

/* Base index into the per-task thread-local storage array for ff_stdio's errno
 * / CWD / FF_Error pointers (offsets +0/+1/+2). Pinned to 0; FreeRTOSConfig.h
 * sizes configNUM_THREAD_LOCAL_STORAGE_POINTERS to cover the three slots. */
#define ffconfigCWD_THREAD_LOCAL_INDEX (0)

/* The store uses absolute 8.3 paths at the volume root (/STORE00.log, …), so
 * relative-path / long-filename support is left at the defaults (both off). */

#endif /* FREERTOSFATCONFIG_H */
