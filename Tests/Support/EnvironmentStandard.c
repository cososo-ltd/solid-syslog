#include "Environment.h"

#include <stdlib.h>

const char* Environment_Get(const char* name)
{
    return getenv(name);
}
