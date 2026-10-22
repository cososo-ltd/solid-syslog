#ifndef BDDTARGETCOMMANDLINE_H
#define BDDTARGETCOMMANDLINE_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogPrival.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct BddTargetOptions
    {
        enum SolidSyslogFacility Facility;
        enum SolidSyslogSeverity Severity;
        const char* MessageId;
        const char* Msg;
        const char* AppName; /* --app-name (NULL: derive from argv[0]) */
        const char* Transport;
        const char* Store;
        const char* ErrorsFatal; /* "1" (default) | "0" - whether an ERROR ends the target */
    };

    /* The store options (--max-blocks, --max-block-size, --discard-policy,
       --security-policy, --capacity-threshold, --halt-exit, --no-sd) go to
       BddTargetStoreSettings, which the parse resets first. Returns 0, or 1
       when an option's argument is not one it accepts. */
    int BddTargetCommandLine_Parse(int argc, char* argv[], struct BddTargetOptions* options);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETCOMMANDLINE_H */
