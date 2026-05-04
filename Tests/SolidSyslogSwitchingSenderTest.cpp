#include <stddef.h>
#include <stdint.h>

#include "SenderFake.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogSwitchingSender.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for CALLED_FUNCTION

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
        SolidSyslogSwitchingSender_Destroy();
        SenderFake_Destroy(innerC);
        SenderFake_Destroy(innerB);
        SenderFake_Destroy(innerA);
    }

    void CreateSwitchingSender(size_t count)
    {
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
    SolidSyslogSwitchingSender_Destroy();
    CALLED_FUNCTION(SenderFake_SendCount(innerA), NEVER);
    CALLED_FUNCTION(SenderFake_SendCount(innerB), NEVER);
}

TEST(SolidSyslogSwitchingSender, DestroyDoesNotDisconnectInnerSenders)
{
    SolidSyslogSwitchingSender_Destroy();
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerA), NEVER);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerB), NEVER);
}

TEST(SolidSyslogSwitchingSender, SendDelegatesToSenderAtSelectedIndex)
{
    Send("x", 1);
    CALLED_FUNCTION(SenderFake_SendCount(innerA), ONCE);
    CALLED_FUNCTION(SenderFake_SendCount(innerB), NEVER);
}

TEST(SolidSyslogSwitchingSender, SecondSendAtSameIndexDoesNotDisconnect)
{
    Send("x", 1);
    Send("y", 1);
    CALLED_FUNCTION(SenderFake_SendCount(innerA), TWICE);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerA), NEVER);
}

TEST(SolidSyslogSwitchingSender, SelectorChangeDisconnectsOutgoingAndSendsOnIncoming)
{
    Send("x", 1);
    selectorReturn = INNER_B;
    Send("y", 1);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerA), ONCE);
    CALLED_FUNCTION(SenderFake_SendCount(innerA), ONCE);
    CALLED_FUNCTION(SenderFake_SendCount(innerB), ONCE);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerB), NEVER);
}

TEST(SolidSyslogSwitchingSender, SteadyStateAfterSwitchDoesNotDisconnectAgain)
{
    Send("x", 1);
    selectorReturn = INNER_B;
    Send("y", 1);
    Send("z", 1);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerA), ONCE);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerB), NEVER);
    CALLED_FUNCTION(SenderFake_SendCount(innerB), TWICE);
}

TEST(SolidSyslogSwitchingSender, DisconnectBeforeAnySendDoesNotTouchInnerSenders)
{
    SolidSyslogSender_Disconnect(sender);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerA), NEVER);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerB), NEVER);
}

TEST(SolidSyslogSwitchingSender, DisconnectForwardsToCurrentSender)
{
    Send("x", 1);
    SolidSyslogSender_Disconnect(sender);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerA), ONCE);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerB), NEVER);
}

TEST(SolidSyslogSwitchingSender, DisconnectAfterSwitchForwardsToNewActive)
{
    Send("x", 1);
    selectorReturn = INNER_B;
    Send("y", 1);
    SolidSyslogSender_Disconnect(sender);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerA), ONCE);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerB), ONCE);
}

TEST(SolidSyslogSwitchingSender, DisconnectAfterSelectorChangeWithoutSendForwardsToPreviouslyActive)
{
    Send("x", 1);             // currentSender becomes innerA
    selectorReturn = INNER_B; // selector flips, but no Send yet
    SolidSyslogSender_Disconnect(sender);
    // Disconnect does not re-consult the selector — it forwards to the
    // currently-held sender, so innerA receives the Disconnect.
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerA), ONCE);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerB), NEVER);
}

TEST(SolidSyslogSwitchingSender, SelectorReturningNonZeroOnFirstSendLandsOnThatIndex)
{
    selectorReturn = INNER_B;
    Send("x", 1);
    CALLED_FUNCTION(SenderFake_SendCount(innerA), NEVER);
    CALLED_FUNCTION(SenderFake_SendCount(innerB), ONCE);
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
    CALLED_FUNCTION(SenderFake_SendCount(innerA), NEVER);
    CALLED_FUNCTION(SenderFake_SendCount(innerB), NEVER);
    CALLED_FUNCTION(SenderFake_SendCount(innerC), ONCE);
}

TEST(SolidSyslogSwitchingSender, ZeroSenderCountSendReturnsFalse)
{
    CreateSwitchingSender(0);
    CHECK_FALSE(SolidSyslogSender_Send(sender, "x", 1));
}

TEST(SolidSyslogSwitchingSender, ZeroSenderCountDisconnectDoesNotCrash)
{
    CreateSwitchingSender(0);
    SolidSyslogSender_Disconnect(sender);
}

TEST(SolidSyslogSwitchingSender, SelectorBeyondEndSendReturnsFalseAndDoesNotTouchInnerSenders)
{
    selectorReturn = BEYOND_END;
    CHECK_FALSE(SolidSyslogSender_Send(sender, "x", 1));
    CALLED_FUNCTION(SenderFake_SendCount(innerA), NEVER);
    CALLED_FUNCTION(SenderFake_SendCount(innerB), NEVER);
}

TEST(SolidSyslogSwitchingSender, DisconnectAfterSwitchingBeyondEndIsNilSafe)
{
    Send("x", 1);
    selectorReturn = BEYOND_END;
    Send("y", 1);
    // innerA was active, switched away — one Disconnect from the switch
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerA), ONCE);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerB), NEVER);
    // explicit Disconnect now resolves to nil — no inner sender touched
    SolidSyslogSender_Disconnect(sender);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerA), ONCE);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerB), NEVER);
}

TEST(SolidSyslogSwitchingSender, SelectorBeyondEndDisconnectBeforeSendDoesNotTouchInnerSenders)
{
    selectorReturn = BEYOND_END;
    SolidSyslogSender_Disconnect(sender);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerA), NEVER);
    CALLED_FUNCTION(SenderFake_DisconnectCount(innerB), NEVER);
}
