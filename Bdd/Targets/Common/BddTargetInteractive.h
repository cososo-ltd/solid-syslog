#ifndef BDDTARGETINTERACTIVE_H
#define BDDTARGETINTERACTIVE_H

#include <stdbool.h>
#include <stdio.h>

#include "ExternC.h"

struct SolidSyslog;
struct SolidSyslogMessage;

EXTERN_C_BEGIN

    typedef void (*BddTargetInteractiveSwitchHandler)(const char* name);
    typedef bool (*BddTargetInteractiveSetHandler)(const char* name, const char* value);

    void BddTargetInteractive_Run(
        struct SolidSyslog * handle,
        const struct SolidSyslogMessage* message,
        FILE* input,
        BddTargetInteractiveSwitchHandler onSwitch,
        BddTargetInteractiveSetHandler onSet
    );

EXTERN_C_END

#endif /* BDDTARGETINTERACTIVE_H */
