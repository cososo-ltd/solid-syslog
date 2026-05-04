#ifndef EXAMPLEINTERACTIVE_H
#define EXAMPLEINTERACTIVE_H

#include <stdio.h>

#include "ExternC.h"

struct SolidSyslogMessage;

EXTERN_C_BEGIN

    typedef void (*ExampleInteractiveSwitchHandler)(const char* name);

    void ExampleInteractive_Run(const struct SolidSyslogMessage* message, FILE* input, ExampleInteractiveSwitchHandler onSwitch);

EXTERN_C_END

#endif /* EXAMPLEINTERACTIVE_H */
