#ifndef BDDTARGETWINDOWSCOMMANDLINE_H
#define BDDTARGETWINDOWSCOMMANDLINE_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogPrival.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct BddTargetWindowsOptions
    {
        enum SolidSyslogFacility Facility;
        enum SolidSyslogSeverity Severity;
        const char* Transport; /* "udp" | "tcp" | "tls" | "mtls" - initial selector */
        const char* MessageId;
        const char* Msg;
        const char* AppName; /* --app-name (NULL: derive from argv[0]) */
        const char* Store; /* "null" (default) | "file" - block-store backend */
        const char* ErrorsFatal; /* --errors-fatal 1|0 - whether an ERROR ends the target */
    };

    /* Minimal CLI parser - recognises the flags below. Unknown flags, flags
       missing their argument and a store value the shared settings refuse are
       silently ignored.
         --facility N
         --severity N
         --msgid X
         --message X
         --transport udp|tcp|tls|mtls    (default: udp)
         --app-name X                    (default: derive from argv[0])
         --store null|file               (default: null)
         --errors-fatal 1|0              (default: 1 - an ERROR ends the target)
       The store options (--max-blocks, --max-block-size, --discard-policy,
       --security-policy, --capacity-threshold, --halt-exit, --no-sd) go to
       BddTargetStoreSettings, which the parse resets first; the bare flags
       pass as 1.
       getopt is not available on MSVC and pulling in a vcpkg getopt for a
       handful of flags would be overkill. */
    void BddTargetWindowsCommandLine_Parse(int argc, char* argv[], struct BddTargetWindowsOptions* options);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETWINDOWSCOMMANDLINE_H */
