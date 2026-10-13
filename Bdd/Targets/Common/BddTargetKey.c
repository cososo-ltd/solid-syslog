#include "BddTargetKey.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum
{
    DEMO_KEY_SIZE = 32
};

static bool withheld;

void BddTargetKey_Reset(void)
{
    withheld = false;
}

bool BddTargetKey_SetByName(const char* name, const char* value)
{
    bool applied = false;
    if (strcmp(name, "key") == 0)
    {
        applied = (strcmp(value, "withheld") == 0) || (strcmp(value, "available") == 0);
        if (applied)
        {
            withheld = (strcmp(value, "withheld") == 0);
        }
    }
    return applied;
}

bool BddTargetKey_Get(void* context, uint8_t* keyOut, size_t capacity, size_t* keyLengthOut)
{
    (void) context;
    bool available = !withheld;
    if (available)
    {
        size_t written = (capacity < DEMO_KEY_SIZE) ? capacity : (size_t) DEMO_KEY_SIZE;
        (void) memset(keyOut, 0x5A, written);
        *keyLengthOut = written;
    }
    return available;
}
