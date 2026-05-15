#ifndef BDDTARGETCOMMANDLINE_H
#define BDDTARGETCOMMANDLINE_H

#include "ExternC.h"
#include "SolidSyslogPrival.h"

#include <stdbool.h>
#include <stddef.h>

EXTERN_C_BEGIN

    struct BddTargetOptions
    {
        enum SolidSyslogFacility facility;
        enum SolidSyslogSeverity severity;
        const char* messageId;
        const char* msg;
        const char* appName; /* --app-name (NULL: derive from argv[0]) */
        const char* transport;
        const char* store;
        size_t maxBlocks;
        size_t maxBlockSize;
        const char* discardPolicy;
        size_t capacityThreshold;
        bool noSd;
        bool haltExit;
    };

    int BddTargetCommandLine_Parse(int argc, char* argv[], struct BddTargetOptions* options);

EXTERN_C_END

#endif /* BDDTARGETCOMMANDLINE_H */
