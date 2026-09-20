#ifndef BDD_TARGET_FREE_RTOS_PIPELINE_H
#define BDD_TARGET_FREE_RTOS_PIPELINE_H

#include "SolidSyslogEndpoint.h"
#include "SolidSyslogHeaderFieldFunction.h"
#include "SolidSyslogSender.h"

#include <stdbool.h>
#include <stdint.h>

/* Shared FreeRTOS BDD-target pipeline.
 *
 * Everything platform-independent lives in this component: the SolidSyslog
 * lifecycle, the file-backed store + security-policy machinery (crc16 /
 * hmac-sha256 / aes-256-gcm / null), the SD set, the interactive `set` handler,
 * the CircularBuffer + Service drain task, and the console glue. Every QEMU BDD
 * target drives this; their main.c files keep only the network backend and the
 * FS-mount seam behind the config below - the network adapter wiring (PlusTcp vs
 * LwipRaw), the IP-stack bring-up, and the filesystem vendor (FreeRTOS-Plus-FAT
 * vs ChaN-FatFs) that genuinely differ. See SolidSyslog S29.03 (network seam)
 * and S29.05 (FS-mount seam). The OS calls themselves go through
 * BddTargetOsPrimitives.h, so nothing here names a kernel. */

/* Forward declaration - the FS-mount seam traffics in SolidSyslogFile handles
 * without the pipeline header pulling SolidSyslogFile.h. */
struct SolidSyslogFile;

/* The platform seam each target injects via BddTargetFreeRtosPipeline_SetConfig. */
struct BddTargetFreeRtosPipelineConfig
{
    /* Default destination host before any `set host` - numeric for the no-DNS
     * PlusTcp target ("10.0.2.2"); the DNS alias for lwIP ("syslog-ng"). */
    const char* DefaultHost;
    /* Bring up the platform network and build the (Switching) sender, with the
     * default transport already selected. Runs on the interactive task, so an
     * adapter that must touch a started IP stack (LwipRaw) is safe here. */
    struct SolidSyslogSender* (*BuildSender)(void);
    /* Emit the RFC 5424 HOSTNAME by reading the platform IP stack. Wired into
     * SolidSyslogConfig.GetHostname. */
    SolidSyslogHeaderFieldFunction GetHostname;
    /* Tear down the sender + platform adapters. Runs on the interactive task
     * after the shared pipeline teardown (SolidSyslog / SD / store / buffer). */
    void (*TeardownNetwork)(void);

    /* FS-mount seam - the FS-vendor-specific half of the file-backed store.
     * The Plus-TCP target wires the FreeRTOS-Plus-FAT shim (BddTargetPlusFatMount);
     * the lwIP target wires the ChaN-FatFs shim (BddTargetFatFsMount). The
     * pipeline drives the store machinery (BlockStore / FileBlockDevice /
     * security policy) FS-vendor-agnostically through these four hooks. */
    /* Mount + format-on-first-use; idempotent; false on unrecoverable failure
     * (the rebuild path then leaves the target on its current store). */
    bool (*MountStore)(void);
    /* Unmount on teardown; safe to call when not mounted. */
    void (*UnmountStore)(void);
    /* Create / destroy the platform SolidSyslogFile adapter for the store. */
    struct SolidSyslogFile* (*CreateStoreFile)(void);
    void (*DestroyStoreFile)(struct SolidSyslogFile* file);
};

/* Install the platform seam. Call once from main() before the tasks run. */
void BddTargetFreeRtosPipeline_SetConfig(const struct BddTargetFreeRtosPipelineConfig* config);

/* Endpoint callbacks (SolidSyslogEndpointFunction / ...VersionFunction shaped),
 * reading the shared host/port that `set host` / `set port` rewrite. A target's
 * BuildSender wires these into its UdpSender / StreamSender configs. */
void BddTargetFreeRtosPipeline_GetEndpoint(struct SolidSyslogEndpoint* endpoint, void* context);
uint32_t BddTargetFreeRtosPipeline_GetEndpointVersion(void* context);

/* Initialise the CMSDK UART console with the shared MMIO access. */
void BddTargetFreeRtosPipeline_InitConsole(uint32_t uartBaseAddress);

/* Shared bounded-spin sleep (SolidSyslogSleepFunction-shaped). The lwIP target
 * passes this into its resolver / TCP-stream config; both targets reuse it for
 * the CMSDK UART yield. */
void BddTargetFreeRtosPipeline_Sleep(int milliseconds);

/* ARM Semihosting SYS_EXIT - terminates QEMU with the given status. Used
 * internally by the `shutdown` / halt paths; exposed so a target's main() can
 * bail out on an unrecoverable bring-up failure (e.g. xTaskCreate). */
void BddTargetFreeRtosPipeline_Exit(int status);

/* Stack sizes in bytes, the unit BddTargetOsPrimitives_Spawn takes. Each
 * main.c spawns the threads itself so that it controls the timing - the
 * PlusTcp target from its network-up hook, the lwIP targets from main(). */
#define BDD_TARGET_INTERACTIVE_STACK_BYTES 24576U
#define BDD_TARGET_SERVICE_STACK_BYTES 8192U

/* Entry points for the two shared threads. The Service thread self-registers
 * its handle, so neither needs the caller to plumb anything. */
void BddTargetFreeRtosPipeline_InteractiveTask(void* argument);
void BddTargetFreeRtosPipeline_ServiceTask(void* argument);

#endif /* BDD_TARGET_FREE_RTOS_PIPELINE_H */
