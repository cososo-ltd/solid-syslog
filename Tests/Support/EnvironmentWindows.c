#include "Environment.h"

#include <stdlib.h>

enum
{
    ENVIRONMENT_VALUE_CAPACITY = 256
};

const char* Environment_Get(const char* name)
{
    static char value[ENVIRONMENT_VALUE_CAPACITY];
    size_t requiredSize = 0U;
    const char* result = NULL;

    if ((getenv_s(&requiredSize, value, sizeof(value), name) == 0) && (requiredSize > 0U))
    {
        result = value;
    }

    return result;
}
