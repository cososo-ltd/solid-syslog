#ifndef BDDTARGETCOMMANDLINE_H
#define BDDTARGETCOMMANDLINE_H

#include "ExternC.h"
#include "SolidSyslogPrival.h"

#include <stdbool.h>
#include <stddef.h>

EXTERN_C_BEGIN

    struct BddTargetOptions
    {
        enum SolidSyslogFacility Facility;
        enum SolidSyslogSeverity Severity;
        const char* MessageId;
        const char* Msg;
        const char* AppName; /* --app-name (NULL: derive from argv[0]) */
        const char* Transport;
        const char* Store;
        size_t MaxBlocks;
        size_t MaxBlockSize;
        const char* DiscardPolicy;
        const char* SecurityPolicy; /* "crc16" (default) | "hmac-sha256" | "null" — at-rest integrity */
        size_t CapacityThreshold;
        bool NoSd;
        bool HaltExit;
    };

    int BddTargetCommandLine_Parse(int argc, char* argv[], struct BddTargetOptions* options);

EXTERN_C_END

#endif /* BDDTARGETCOMMANDLINE_H */
