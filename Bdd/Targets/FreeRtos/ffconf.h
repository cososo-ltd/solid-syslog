/* FatFs integrator configuration for the FreeRTOS BDD target on QEMU
 * mps2-an385. FF_VOLUMES=1 — single logical drive on the semihosting
 * disk image; FF_USE_MKFS=1 so disk_initialize can fall through to
 * f_mkfs when the image is fresh (after_scenario deletes
 * solidsyslog-disk.img between scenarios, so every scenario starts
 * with no filesystem and the integrator formats on first mount).
 *
 * FF_FS_REENTRANT=1 even though only the Service task touches the
 * store today — the future reentrancy stress test exercises the
 * production lock path with mutexes already in place rather than
 * having to wire them in later. ffsystem.c uses OS_TYPE 3 (FreeRTOS
 * dynamic xSemaphoreCreateMutex); see project_s08_05_fatfs_reentrancy_decision
 * for the trade-off vs SolidSyslogMutex. */

#define FFCONF_DEF 80386 /* Revision ID — must match FF_DEFINED in ff.h */

/* Function Configurations */
#define FF_FS_READONLY 0
#define FF_FS_MINIMIZE 0
#define FF_USE_FIND 0
#define FF_USE_MKFS 1 /* Integrator falls through to f_mkfs on FR_NO_FILESYSTEM. */
#define FF_USE_FASTSEEK 0
#define FF_USE_EXPAND 0
#define FF_USE_CHMOD 0
#define FF_USE_LABEL 0
#define FF_USE_FORWARD 0
#define FF_USE_STRFUNC 0
#define FF_PRINT_LLI 0
#define FF_PRINT_FLOAT 0
#define FF_STRF_ENCODE 0

/* Locale and Namespace */
#define FF_CODE_PAGE 437 /* Latin1 — avoids pulling in DBCS sub-tables when LFN=0. */
#define FF_USE_LFN 0     /* Short 8.3 filenames; pathPrefix scheme fits (e.g. STORE00.LOG). */
#define FF_MAX_LFN 255
#define FF_LFN_UNICODE 0
#define FF_LFN_BUF 255
#define FF_SFN_BUF 12
#define FF_FS_RPATH 0
#define FF_PATH_DEPTH 10

/* Drive/Volume */
#define FF_VOLUMES 1
#define FF_STR_VOLUME_ID 0
#define FF_VOLUME_STRS "RAM", "NAND", "CF", "SD", "SD2", "USB", "USB2", "USB3"
#define FF_MULTI_PARTITION 0
#define FF_MIN_SS 512
#define FF_MAX_SS 512
#define FF_LBA64 0
#define FF_MIN_GPT 0x10000000
#define FF_USE_TRIM 0

/* System */
#define FF_FS_TINY 0
#define FF_FS_EXFAT 0
#define FF_FS_NORTC 1 /* No RTC on the QEMU target — fixed-date stamps. */
#define FF_NORTC_MON 1
#define FF_NORTC_MDAY 1
#define FF_NORTC_YEAR 2026
#define FF_FS_CRTIME 0
#define FF_FS_NOFSINFO 0
#define FF_FS_LOCK 0
#define FF_FS_REENTRANT 1 /* Forward-looking — exercise the lock path before stress tests need it. */
#define FF_FS_TIMEOUT 1000
