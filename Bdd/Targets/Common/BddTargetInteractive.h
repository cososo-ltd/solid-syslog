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
    /* Runs after each record a send logs, on the task that logged it: where a
       single-task wiring services the logger, since it has no other task to. */
    typedef void (*BddTargetInteractiveLoggedHandler)(struct SolidSyslog* handle);

    void BddTargetInteractive_Run(
        struct SolidSyslog * handle,
        const struct SolidSyslogMessage* message,
        FILE* input,
        BddTargetInteractiveSwitchHandler onSwitch,
        BddTargetInteractiveSetHandler onSet,
        BddTargetInteractiveBlocksHandler onBlocks,
        BddTargetInteractiveLoggedHandler onLogged
    );

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETINTERACTIVE_H */
