#ifndef BDD_TARGET_FREE_RTOS_PIPELINE_H
#define BDD_TARGET_FREE_RTOS_PIPELINE_H

#include "SolidSyslogEndpoint.h"
#include "SolidSyslogHeaderFieldFunction.h"
#include "SolidSyslogSender.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Shared FreeRTOS BDD-target pipeline.
 *
 * Everything platform-independent lives in this component: the SolidSyslog
 * lifecycle, the file-backed store + security-policy machinery (crc16 /
 * hmac-sha256 / aes-256-gcm / null), the SD set, the interactive `set` handler,
 * the CircularBuffer + Service drain task, and the console glue. Each target's
 * main.c keeps only what differs, behind the config below: the network adapter
 * wiring, the IP-stack bring-up and the filesystem mount. The OS calls go
 * through BddTargetOsPrimitives.h, so nothing here names a kernel. */

/* Forward declarations - the store seam traffics in SolidSyslogFile and
 * SolidSyslogBlockDevice handles without the pipeline header pulling either
 * header. */
struct SolidSyslogFile;
struct SolidSyslogBlockDevice;

/* The platform seam each target injects via BddTargetFreeRtosPipeline_SetConfig. */
struct BddTargetFreeRtosPipelineConfig
{
    /* Default destination host before any `set host` - numeric for the no-DNS
     * PlusTcp target, where the board reaches the oracle; the DNS alias for lwIP
     * ("syslog-ng"). */
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
     * security policy) FS-vendor-agnostically through these five hooks. */
    /* Mount + format-on-first-use; idempotent; false on unrecoverable failure
     * (the rebuild path then leaves the target on its current store). */
    bool (*MountStore)(void);
    /* Unmount on teardown; safe to call when not mounted. */
    void (*UnmountStore)(void);
    /* Create / destroy the platform SolidSyslogFile adapter for the store. */
    struct SolidSyslogFile* (*CreateStoreFile)(void);
    void (*DestroyStoreFile)(struct SolidSyslogFile* file);
    /* Leaves the store's medium with no filesystem on it, so the next mount
     * formats it. For a medium that keeps its contents from one run of the
     * target to the next; only called with nothing mounted. Returns false if
     * the medium is cleared some other way, or the change fails. */
    bool (*WipeStore)(void);

    /* For a target whose store is not kept in files: the store's block
     * device, made once MountStore has answered true, with whatever block
     * size BddTargetStoreSettings holds. Both are NULL on a target with a
     * filesystem, and the pipeline then makes a SolidSyslogFileBlockDevice
     * over CreateStoreFile's file. A target that sets them leaves
     * CreateStoreFile and DestroyStoreFile NULL. */
    struct SolidSyslogBlockDevice* (*CreateStoreBlockDevice)(void);
    void (*DestroyStoreBlockDevice)(struct SolidSyslogBlockDevice* device);

    /* For a target that counts what its store's medium is asked to do: writes
     * the counts into text as one line with no newline, and answers false
     * while it has none to give. `set store report` prints the line. NULL on
     * a target that does not count, where that command is refused. */
    bool (*DescribeStoreMedium)(char* text, size_t size);
};

/* Install the platform seam. Call once from main() before the tasks run. */
void BddTargetFreeRtosPipeline_SetConfig(const struct BddTargetFreeRtosPipelineConfig* config);

/* Endpoint callbacks (SolidSyslogEndpointFunction / ...VersionFunction shaped),
 * reading the shared host/port that `set host` / `set port` rewrite. A target's
 * BuildSender wires these into its UdpSender / StreamSender configs. */
void BddTargetFreeRtosPipeline_GetEndpoint(struct SolidSyslogEndpoint* endpoint, void* context);
uint32_t BddTargetFreeRtosPipeline_GetEndpointVersion(void* context);

/* Shared bounded-spin sleep (SolidSyslogSleepFunction-shaped). The lwIP target
 * passes this into its resolver / TCP-stream config. */
void BddTargetFreeRtosPipeline_Sleep(uint32_t milliseconds);

/* Stack sizes in bytes, the unit BddTargetOsPrimitives_Spawn takes. Each
 * main.c spawns the threads itself so that it controls the timing - the
 * PlusTcp target from its network-up hook, the lwIP targets from main(). The
 * service thread carries the TLS handshake, so a build whose TLS library needs
 * more says so on its command line. */
#define BDD_TARGET_INTERACTIVE_STACK_BYTES 24576U
#ifndef BDD_TARGET_SERVICE_STACK_BYTES
#define BDD_TARGET_SERVICE_STACK_BYTES 8192U
#endif

/* Entry points for the two shared threads. The Service thread self-registers
 * its handle, so neither needs the caller to plumb anything. */
void BddTargetFreeRtosPipeline_InteractiveTask(void* argument);
void BddTargetFreeRtosPipeline_ServiceTask(void* argument);

#endif /* BDD_TARGET_FREE_RTOS_PIPELINE_H */
