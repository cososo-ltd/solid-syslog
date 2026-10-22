#ifndef BDDTARGETINTERACTIVE_H
#define BDDTARGETINTERACTIVE_H

#include <stdbool.h>
#include <stdio.h>

#include "SolidSyslogExternC.h"

struct SolidSyslog;
struct SolidSyslogMessage;

SOLIDSYSLOG_EXTERN_C_BEGIN

    typedef void (*BddTargetInteractiveSwitchHandler)(const char* name);
    typedef bool (*BddTargetInteractiveSetHandler)(const char* name, const char* value);
    /* Answers the harness's `blocks` command, over whatever the target has to
       lock to read its store's block files safely. */
    typedef void (*BddTargetInteractiveBlocksHandler)(void);

    void BddTargetInteractive_Run(
        struct SolidSyslog * handle,
        const struct SolidSyslogMessage* message,
        FILE* input,
        BddTargetInteractiveSwitchHandler onSwitch,
        BddTargetInteractiveSetHandler onSet,
        BddTargetInteractiveBlocksHandler onBlocks
    );

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETINTERACTIVE_H */
