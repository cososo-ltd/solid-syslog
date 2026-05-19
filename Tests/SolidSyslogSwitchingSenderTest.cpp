#include <stddef.h>
#include <stdint.h>

#include "ErrorHandlerFake.h"
#include "SenderFake.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros

/* Selector return values — named for the inner sender they select, so tests
 * read as `selectorReturn = INNER_B`. */
enum
{
    INNER_A = 0,
    INNER_B = 1,
    INNER_C = 2,
};

/* Boundary sentinel for the default 2-sender fixture: equals senderCount,
 * the first index past the end. Using this exact value (rather than any
 * large invalid number) is what catches off-by-one errors in the bounds
 * check. Declared separately so it does not sit alongside INNER_C with
 * the same numeric value. */
enum
{
    BEYOND_END = 2,
};

static uint8_t selectorReturn;

static uint8_t TestSelector() // NOLINT(modernize-redundant-void-arg) -- matches C callback signature
{
    return selectorReturn;
}

// clang-format off
TEST_GROUP(SolidSyslogSwitchingSender)
{
    struct SolidSyslogSender* innerA = nullptr;
    struct SolidSyslogSender* innerB = nullptr;
    struct SolidSyslogSender* innerC = nullptr;
    struct SolidSyslogSender* inners[3] = {nullptr, nullptr, nullptr};
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogSender* sender = nullptr;

    void setup() override
    {
        selectorReturn = INNER_A;
        innerA = SenderFake_Create();
        innerB = SenderFake_Create();
        innerC = SenderFake_Create();
        inners[0] = innerA;
        inners[1] = innerB;
        inners[2] = innerC;
        CreateSwitchingSender(2);
    }

    void teardown() override
    {
        SolidSyslogSwitchingSender_Destroy(sender);
        SenderFake_Destroy(innerC);
        SenderFake_Destroy(innerB);
        SenderFake_Destroy(innerA);
    }

    void CreateSwitchingSender(size_t count)
    {
        /* Pool semantics: if setup already created one (slot-allocated), destroy
         * it before creating again so this call reuses the same slot rather than
         * exhausting the pool and getting the Fallback. */
        if (sender != nullptr)
        {
            SolidSyslogSwitchingSender_Destroy(sender);
        }
        struct SolidSyslogSwitchingSenderConfig config = {inners, count, TestSelector};
        sender                                         = SolidSyslogSwitchingSender_Create(&config);
    }

    void Send(const void* buffer, size_t size) const
    {
        SolidSyslogSender_Send(sender, buffer, size);
    }
};

// clang-format on

TEST(SolidSyslogSwitchingSender, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogSwitchingSender, DestroyDoesNotSendToInnerSenders)
{
    SolidSyslogSwitchingSender_Destroy(sender);
    sender = nullptr;

    CALLED_FAKE_ON(SenderFake_Send, innerA, NEVER);
    CALLED_FAKE_ON(SenderFake_Send, innerB, NEVER);

    // Re-create so teardown's Destroy(sender) targets a live handle —
    // the destroy under test has already freed the original slot.
    CreateSwitchingSender(2);
}

TEST(SolidSyslogSwitchingSender, DestroyDoesNotDisconnectInnerSenders)
{
    SolidSyslogSwitchingSender_Destroy(sender);
    sender = nullptr;

    CALLED_FAKE_ON(SenderFake_Disconnect, innerA, NEVER);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerB, NEVER);

    // Re-create so teardown's Destroy(sender) targets a live handle —
    // the destroy under test has already freed the original slot.
    CreateSwitchingSender(2);
}

TEST(SolidSyslogSwitchingSender, UseAfterDestroyIsCrashSafeViaNullSenderVtable)
{
    /* After Destroy the slot's abstract-base vtable is the shared NullSender's, so
     * calling Send/Disconnect through the stale handle is a safe no-op rather than a
     * NULL-fn-pointer crash. NullSender.Send returns true (drop-on-floor). */
    struct SolidSyslogSender* destroyed = sender;
    SolidSyslogSwitchingSender_Destroy(destroyed);
    sender = nullptr;

    CHECK_TRUE(SolidSyslogSender_Send(destroyed, "x", 1));
    SolidSyslogSender_Disconnect(destroyed);
    CALLED_FAKE_ON(SenderFake_Send, innerA, NEVER);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerA, NEVER);

    // Re-create so teardown's Destroy(sender) targets a live handle.
    CreateSwitchingSender(2);
}

TEST(SolidSyslogSwitchingSender, SendDelegatesToSenderAtSelectedIndex)
{
    Send("x", 1);
    CALLED_FAKE_ON(SenderFake_Send, innerA, ONCE);
    CALLED_FAKE_ON(SenderFake_Send, innerB, NEVER);
}

TEST(SolidSyslogSwitchingSender, SecondSendAtSameIndexDoesNotDisconnect)
{
    Send("x", 1);
    Send("y", 1);
    CALLED_FAKE_ON(SenderFake_Send, innerA, TWICE);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerA, NEVER);
}

TEST(SolidSyslogSwitchingSender, SelectorChangeDisconnectsOutgoingAndSendsOnIncoming)
{
    Send("x", 1);
    selectorReturn = INNER_B;
    Send("y", 1);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerA, ONCE);
    CALLED_FAKE_ON(SenderFake_Send, innerA, ONCE);
    CALLED_FAKE_ON(SenderFake_Send, innerB, ONCE);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerB, NEVER);
}

TEST(SolidSyslogSwitchingSender, SteadyStateAfterSwitchDoesNotDisconnectAgain)
{
    Send("x", 1);
    selectorReturn = INNER_B;
    Send("y", 1);
    Send("z", 1);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerA, ONCE);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerB, NEVER);
    CALLED_FAKE_ON(SenderFake_Send, innerB, TWICE);
}

TEST(SolidSyslogSwitchingSender, DisconnectBeforeAnySendDoesNotTouchInnerSenders)
{
    SolidSyslogSender_Disconnect(sender);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerA, NEVER);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerB, NEVER);
}

TEST(SolidSyslogSwitchingSender, DisconnectForwardsToCurrentSender)
{
    Send("x", 1);
    SolidSyslogSender_Disconnect(sender);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerA, ONCE);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerB, NEVER);
}

TEST(SolidSyslogSwitchingSender, DisconnectAfterSwitchForwardsToNewActive)
{
    Send("x", 1);
    selectorReturn = INNER_B;
    Send("y", 1);
    SolidSyslogSender_Disconnect(sender);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerA, ONCE);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerB, ONCE);
}

TEST(SolidSyslogSwitchingSender, DisconnectAfterSelectorChangeWithoutSendForwardsToPreviouslyActive)
{
    Send("x", 1); // currentSender becomes innerA
    selectorReturn = INNER_B; // selector flips, but no Send yet
    SolidSyslogSender_Disconnect(sender);
    // Disconnect does not re-consult the selector — it forwards to the
    // currently-held sender, so innerA receives the Disconnect.
    CALLED_FAKE_ON(SenderFake_Disconnect, innerA, ONCE);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerB, NEVER);
}

TEST(SolidSyslogSwitchingSender, SelectorReturningNonZeroOnFirstSendLandsOnThatIndex)
{
    selectorReturn = INNER_B;
    Send("x", 1);
    CALLED_FAKE_ON(SenderFake_Send, innerA, NEVER);
    CALLED_FAKE_ON(SenderFake_Send, innerB, ONCE);
}

TEST(SolidSyslogSwitchingSender, SendForwardsBufferVerbatimToActive)
{
    Send("payload", 7);
    LONGS_EQUAL(7, SenderFake_LastSize(innerA));
    STRCMP_EQUAL("payload", SenderFake_LastBufferAsString(innerA));
}

TEST(SolidSyslogSwitchingSender, SendReturnsActiveSenderResult)
{
    SenderFake_FailNextSend(innerA);
    CHECK_FALSE(SolidSyslogSender_Send(sender, "x", 1));
    CHECK_TRUE(SolidSyslogSender_Send(sender, "y", 1));
}

TEST(SolidSyslogSwitchingSender, SelectorAtLastValidIndexDelegatesToThatSender)
{
    CreateSwitchingSender(3);
    selectorReturn = INNER_C;
    Send("x", 1);
    CALLED_FAKE_ON(SenderFake_Send, innerA, NEVER);
    CALLED_FAKE_ON(SenderFake_Send, innerB, NEVER);
    CALLED_FAKE_ON(SenderFake_Send, innerC, ONCE);
}

TEST(SolidSyslogSwitchingSender, ZeroSenderCountSendReturnsTrueToDropOnTheFloor)
{
    /* Out-of-range selector (or zero-count) resolves to the shared
     * NullSender, whose Send returns true so messages drop rather than
     * accumulate in the Store. */
    CreateSwitchingSender(0);
    CHECK_TRUE(SolidSyslogSender_Send(sender, "x", 1));
}

TEST(SolidSyslogSwitchingSender, ZeroSenderCountDisconnectDoesNotCrash)
{
    CreateSwitchingSender(0);
    SolidSyslogSender_Disconnect(sender);
}

TEST(SolidSyslogSwitchingSender, SelectorBeyondEndSendReturnsTrueAndDoesNotTouchInnerSenders)
{
    /* Out-of-range selector resolves to the shared NullSender, whose
     * Send returns true so messages drop rather than accumulate. */
    selectorReturn = BEYOND_END;
    CHECK_TRUE(SolidSyslogSender_Send(sender, "x", 1));
    CALLED_FAKE_ON(SenderFake_Send, innerA, NEVER);
    CALLED_FAKE_ON(SenderFake_Send, innerB, NEVER);
}

TEST(SolidSyslogSwitchingSender, DisconnectAfterSwitchingBeyondEndIsNilSafe)
{
    Send("x", 1);
    selectorReturn = BEYOND_END;
    Send("y", 1);
    // innerA was active, switched away — one Disconnect from the switch
    CALLED_FAKE_ON(SenderFake_Disconnect, innerA, ONCE);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerB, NEVER);
    // explicit Disconnect now resolves to nil — no inner sender touched
    SolidSyslogSender_Disconnect(sender);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerA, ONCE);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerB, NEVER);
}

TEST(SolidSyslogSwitchingSender, SelectorBeyondEndDisconnectBeforeSendDoesNotTouchInnerSenders)
{
    selectorReturn = BEYOND_END;
    SolidSyslogSender_Disconnect(sender);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerA, NEVER);
    CALLED_FAKE_ON(SenderFake_Disconnect, innerB, NEVER);
}

// Pool tests — prove SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE caps live
// instances and overflow falls back to the shared SolidSyslogNullSender.

// clang-format off
TEST_GROUP(SolidSyslogSwitchingSenderPool)
{
    struct SolidSyslogSender* innerA                                          = nullptr;
    struct SolidSyslogSender* inners[1]                                       = {nullptr};
    struct SolidSyslogSender* pooled[SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE]  = {};
    struct SolidSyslogSender* overflow                                        = nullptr;
    SolidSyslogSwitchingSenderConfig config;

    void setup() override
    {
        innerA   = SenderFake_Create();
        inners[0] = innerA;
        config   = {inners, 1, TestSelector};
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogSwitchingSender_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogSwitchingSender_Destroy(overflow);
        }
        SenderFake_Destroy(innerA);
    }

    struct SolidSyslogSender* MakeSender()
    {
        return SolidSyslogSwitchingSender_Create(&config);
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = MakeSender();
        }
    }
};

// clang-format on

TEST(SolidSyslogSwitchingSenderPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = MakeSender();

    CHECK_TEXT(overflow != nullptr, "Fallback handle was nullptr");
    for (auto* slot : pooled)
    {
        CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");
        CHECK_TEXT(overflow != slot, "Fallback handle collided with a pool slot");
    }
}

// Bad-setup tests — _Create rejects malformed config and routes to NullSender.

// clang-format off
TEST_GROUP(SolidSyslogSwitchingSenderBadSetup)
{
    struct SolidSyslogSender*               innerA = nullptr;
    struct SolidSyslogSender*               inners[1] = {nullptr};
    int                                     sentinel = 0;
    SolidSyslogSwitchingSenderConfig        config;

    void setup() override
    {
        innerA   = SenderFake_Create();
        inners[0] = innerA;
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        config   = {inners, 1, TestSelector};
        ErrorHandlerFake_Install(&sentinel);
    }

    void teardown() override
    {
        ErrorHandlerFake_Uninstall();
        SenderFake_Destroy(innerA);
    }
};

// clang-format on

TEST(SolidSyslogSwitchingSenderBadSetup, CreateWithNullConfigReportsError)
{
    SolidSyslogSwitchingSender_Create(nullptr);
    CHECK_REPORTED_ERROR("SolidSyslogSwitchingSender_Create called with NULL config");
}

TEST(SolidSyslogSwitchingSenderBadSetup, CreateWithNullSendersReportsError)
{
    config.Senders = nullptr;
    SolidSyslogSwitchingSender_Create(&config);
    CHECK_REPORTED_ERROR("SolidSyslogSwitchingSender_Create config.Senders is NULL");
}

TEST(SolidSyslogSwitchingSenderBadSetup, CreateWithNullSelectorReportsError)
{
    config.Selector = nullptr;
    SolidSyslogSwitchingSender_Create(&config);
    CHECK_REPORTED_ERROR("SolidSyslogSwitchingSender_Create config.Selector is NULL");
}

TEST(SolidSyslogSwitchingSenderBadSetup, SendOnBadSetupSenderReturnsTrueAndDrops)
{
    /* Bad-config _Create returns NullSender (Send drops on the floor),
     * so a misconfigured SwitchingSender doesn't fill the Store. */
    struct SolidSyslogSender* badSender = SolidSyslogSwitchingSender_Create(nullptr);
    CHECK_TRUE(SolidSyslogSender_Send(badSender, "x", 1));
}
