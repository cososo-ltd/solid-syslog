#ifndef EXAMPLEIPS_H
#define EXAMPLEIPS_H

#include "ExternC.h"

#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslogFormatter;

    size_t ExampleIps_Count(void); // NOLINT(modernize-redundant-void-arg) -- C idiom
    void   ExampleIps_At(struct SolidSyslogFormatter * formatter, size_t index);

EXTERN_C_END

#endif /* EXAMPLEIPS_H */
