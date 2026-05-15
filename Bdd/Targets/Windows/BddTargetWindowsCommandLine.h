#ifndef BDDTARGETWINDOWSCOMMANDLINE_H
#define BDDTARGETWINDOWSCOMMANDLINE_H

#include "ExternC.h"
#include "SolidSyslogPrival.h"

#include <stdbool.h>
#include <stddef.h>

EXTERN_C_BEGIN

    struct BddTargetWindowsOptions
    {
        enum SolidSyslogFacility facility;
        enum SolidSyslogSeverity severity;
        const char* transport; /* "udp" | "tcp" | "tls" | "mtls" — initial selector */
        const char* messageId;
        const char* msg;
        const char* appName; /* --app-name (NULL: derive from argv[0]) */
        const char* store; /* "null" (default) | "file" — block-store backend */
        size_t maxBlocks; /* --max-blocks */
        size_t maxBlockSize; /* --max-block-size */
        const char* discardPolicy; /* "oldest" (default) | "newest" | "halt" */
        size_t capacityThreshold; /* --capacity-threshold (bytes; 0 disables) */
        bool haltExit; /* --halt-exit */
        bool noSd; /* --no-sd (suppress structured data) */
    };

    /* Minimal CLI parser — recognises the flags below. Unknown flags and
       flags missing their argument are silently ignored. Defaults match the
       Linux Threaded example so BDD scenarios drive both runners with the
       same arguments.
         --facility N
         --severity N
         --msgid X
         --message X
         --transport udp|tcp|tls|mtls    (default: udp)
         --app-name X                    (default: derive from argv[0])
         --store null|file               (default: null)
         --max-blocks N                  (default set by example)
         --max-block-size N              (default set by example)
         --discard-policy oldest|newest|halt   (default: oldest)
         --capacity-threshold N          (default: 0 — disabled)
         --halt-exit                     (flag; default: off — matches the Linux Threaded example so the BDD step's --halt-exit flag works on both runners)
         --no-sd                         (flag; default: off — suppress structured data)
       getopt is not available on MSVC and pulling in a vcpkg getopt for a
       handful of flags would be overkill. */
    void BddTargetWindowsCommandLine_Parse(int argc, char* argv[], struct BddTargetWindowsOptions* options);

EXTERN_C_END

#endif /* BDDTARGETWINDOWSCOMMANDLINE_H */
