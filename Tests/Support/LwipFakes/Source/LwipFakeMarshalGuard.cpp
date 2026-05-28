#include "LwipFakeMarshalGuard.h"

#include <stddef.h>

#include "CppUTest/TestHarness.h"

bool LwipFakeMarshalGuard_Active = false;

static bool guardBreached = false;
static const char* guardBreachFile = nullptr;
static int guardBreachLine = 0;

void LwipFakeMarshalGuard_RequireActive(const char* file, int line)
{
    if (!LwipFakeMarshalGuard_Active && !guardBreached)
    {
        guardBreached = true;
        guardBreachFile = file;
        guardBreachLine = line;
    }
}

void LwipFakeMarshalGuard_Reset(void)
{
    guardBreached = false;
    guardBreachFile = nullptr;
    guardBreachLine = 0;
    LwipFakeMarshalGuard_Active = false;
}

void LwipFakeMarshalGuard_CheckNoBreach(void)
{
    if (guardBreached)
    {
        UtestShell::getCurrent()->fail(
            "lwIP API called outside the marshal — marshalling rail breach",
            guardBreachFile,
            (size_t) guardBreachLine
        );
    }
}

void LwipFakeMarshalGuard_TrackingMarshal(SolidSyslogLwipRawCallback callback, void* context)
{
    /* Save/restore rather than unconditionally clearing, so a nested marshal
     * (callback re-entering the marshal) leaves the outer scope still active
     * on return instead of a false breach. */
    bool wasActive = LwipFakeMarshalGuard_Active;
    LwipFakeMarshalGuard_Active = true;
    callback(context);
    LwipFakeMarshalGuard_Active = wasActive;
}
