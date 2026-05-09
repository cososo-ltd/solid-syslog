#ifndef EXAMPLEINTERACTIVE_H
#define EXAMPLEINTERACTIVE_H

#include <stdbool.h>
#include <stdio.h>

#include "ExternC.h"

struct SolidSyslogMessage;

EXTERN_C_BEGIN

    typedef void (*ExampleInteractiveSwitchHandler)(const char* name);
    typedef bool (*ExampleInteractiveSetHandler)(const char* name, const char* value);

    void ExampleInteractive_Run(const struct SolidSyslogMessage* message, FILE* input, ExampleInteractiveSwitchHandler onSwitch,
                                ExampleInteractiveSetHandler onSet);

EXTERN_C_END

#endif /* EXAMPLEINTERACTIVE_H */
