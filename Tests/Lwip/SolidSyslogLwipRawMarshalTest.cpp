#include "CppUTest/TestHarness.h"

#include "SolidSyslogLwipRawMarshal.h"
#include "SolidSyslogLwipRawMarshalPrivate.h"

static void Increment(void* context)
{
    ++*static_cast<int*>(context);
}

static int customMarshalCalls = 0;
static SolidSyslogLwipRawCallback lastMarshalledCallback = nullptr;
static void* lastMarshalledContext = nullptr;

static void CustomMarshal(SolidSyslogLwipRawCallback callback, void* context)
{
    ++customMarshalCalls;
    lastMarshalledCallback = callback;
    lastMarshalledContext = context;
    callback(context);
}

// clang-format off
TEST_GROUP(SolidSyslogLwipRawMarshal)
{
    void setup() override
    {
        customMarshalCalls = 0;
        lastMarshalledCallback = nullptr;
        lastMarshalledContext = nullptr;
    }

    void teardown() override
    {
        SolidSyslogLwipRaw_SetMarshal(nullptr);
    }
};
// clang-format on

TEST(SolidSyslogLwipRawMarshal, DefaultMarshalInvokesCallbackOnce)
{
    int calls = 0;

    SolidSyslogLwipRaw_Marshal(Increment, &calls);

    LONGS_EQUAL(1, calls);
}

TEST(SolidSyslogLwipRawMarshal, InstalledMarshalReceivesTheDispatch)
{
    int calls = 0;
    SolidSyslogLwipRaw_SetMarshal(CustomMarshal);

    SolidSyslogLwipRaw_Marshal(Increment, &calls);

    LONGS_EQUAL(1, customMarshalCalls);
    LONGS_EQUAL(1, calls);
}

TEST(SolidSyslogLwipRawMarshal, SetMarshalNullRestoresDirectCallDefault)
{
    int calls = 0;
    SolidSyslogLwipRaw_SetMarshal(CustomMarshal);

    SolidSyslogLwipRaw_SetMarshal(nullptr);
    SolidSyslogLwipRaw_Marshal(Increment, &calls);

    LONGS_EQUAL(0, customMarshalCalls);
    LONGS_EQUAL(1, calls);
}

TEST(SolidSyslogLwipRawMarshal, EachDispatchIsOneMarshalHop)
{
    int calls = 0;
    SolidSyslogLwipRaw_SetMarshal(CustomMarshal);

    SolidSyslogLwipRaw_Marshal(Increment, &calls);
    SolidSyslogLwipRaw_Marshal(Increment, &calls);
    SolidSyslogLwipRaw_Marshal(Increment, &calls);

    LONGS_EQUAL(3, customMarshalCalls);
    LONGS_EQUAL(3, calls);
}

TEST(SolidSyslogLwipRawMarshal, MarshalForwardsCallbackAndContextUnchanged)
{
    int calls = 0;
    SolidSyslogLwipRaw_SetMarshal(CustomMarshal);

    SolidSyslogLwipRaw_Marshal(Increment, &calls);

    FUNCTIONPOINTERS_EQUAL(Increment, lastMarshalledCallback);
    POINTERS_EQUAL(&calls, lastMarshalledContext);
}
