/* Host-suitable ffconf.h for compiling Platform/FatFs adapters against
 * the FatFsFakes. Mirrors the upstream /opt/fatfs/source/ffconf.h
 * (revision 80386, R0.16) with overrides documented inline.
 *
 * No adapter source compiles against this at S08.05 slice 1 — the
 * placeholder Tests/FatFs/SolidSyslogFatFsFileTest only includes ff.h to
 * prove the include path resolves. First content lands with slice 2.
 *
 * The integrator BDD-target ffconf.h lives separately under
 * Bdd/Targets/FreeRtos/ — header-configured platform pack, per
 * project_header_configured_platforms memory.
 */

#define FFCONF_DEF 80386 /* Revision ID — must match FF_DEFINED in ff.h */

/* Function Configurations */
#define FF_FS_READONLY 0
#define FF_FS_MINIMIZE 0
#define FF_USE_FIND 0
#define FF_USE_MKFS 0 /* Adapter doesn't format — integrator's job. */
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
#define FF_USE_LFN 0
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
#define FF_FS_NORTC 1 /* Skip get_fattime() — fixed-date stamps fine for fakes. */
#define FF_NORTC_MON 1
#define FF_NORTC_MDAY 1
#define FF_NORTC_YEAR 2026
#define FF_FS_CRTIME 0
#define FF_FS_NOFSINFO 0
#define FF_FS_LOCK 0
#define FF_FS_REENTRANT 0 /* Host TDD is single-threaded. */
#define FF_FS_TIMEOUT 1000
