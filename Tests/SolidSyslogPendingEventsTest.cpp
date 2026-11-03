#include <stddef.h>
#include <stdint.h>

#include "CppUTest/TestHarness.h"

#include "AddressFake.h"
#include "ErrorHandlerFake.h"
#include "ResolverFake.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogSenderCategories.h"
#include "SolidSyslogStoreCategories.h"
#include "SolidSyslogTlsStreamCategories.h"
#include "StreamFake.h"
#include "TestUtils.h"

using namespace CososoTesting;

class TEST_SolidSyslogPendingEventsInsideSend_ADeliveryFailureRaisedInsideSendIsCapturedWithoutRecursion_Test;
#include "SolidSyslogPendingEvents.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogStreamSenderErrors.h"

/* The table is static and has no production reset: a pending kind is meant to
 * survive until something takes it. Tests start clean by taking every slot. */
static void DrainEveryKind(struct SolidSyslogErrorEvent* into)
{
    for (size_t index = 0U; index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT; index++)
    {
        (void) SolidSyslogPendingEvents_Take(index, into);
    }
}

// clang-format off
TEST_GROUP(SolidSyslogPendingEvents)
{
    struct SolidSyslogErrorEvent taken;
    int sentinel = 0;

    void setup() override
    {
        DrainEveryKind(&taken);
        ErrorHandlerFake_Install(&sentinel);
    }

    bool TakeKind(uint16_t category)
    {
        return SolidSyslogPendingEvents_Take(SolidSyslogPendingEvents_IndexOf(category), &taken);
    }
};

// clang-format on

TEST(SolidSyslogPendingEvents, NothingIsPendingOnceEveryKindHasBeenTaken)
{
    for (size_t index = 0U; index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT; index++)
    {
        CHECK_FALSE(SolidSyslogPendingEvents_Take(index, &taken));
    }
}

TEST(SolidSyslogPendingEvents, ARaiseOfAReportedKindFillsItsSlot)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, &source, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, 7);

    CHECK(TakeKind(SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST));
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, taken.Severity);
    POINTERS_EQUAL(&source, taken.Source);
    LONGS_EQUAL(SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, taken.Category);
    LONGS_EQUAL(7, taken.Detail);
    CHECK_FALSE(taken.HasNativeCode);
}

TEST(SolidSyslogPendingEvents, ARaiseWithANativeCodeKeepsItInTheSlot)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    SolidSyslog_ErrorWithNativeCode(SOLIDSYSLOG_SEVERITY_ERROR, &source, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, 7, -5);

    CHECK(TakeKind(SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST));
    CHECK(taken.HasNativeCode);
    LONGS_EQUAL(-5, taken.NativeCode);
}

TEST(SolidSyslogPendingEvents, ASecondRaiseOfTheSameKindOverwritesTheDetailAndStaysPending)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, &source, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, 7);
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &source, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, 9);

    CHECK(TakeKind(SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST));
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, taken.Severity);
    LONGS_EQUAL(9, taken.Detail);
}

TEST(SolidSyslogPendingEvents, ACategoryOutsideTheSetChangesNothing)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_CRITICAL, &source, SOLIDSYSLOG_CAT_BAD_CONFIG, 1);
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_CRITICAL, &source, 0xC000U, 1);

    for (size_t index = 0U; index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT; index++)
    {
        CHECK_FALSE(SolidSyslogPendingEvents_Take(index, &taken));
    }
}

static const uint16_t REPORTED_KINDS[SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT] = {
    SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
    SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
    SOLIDSYSLOG_CAT_STORE_RECORDS_LOST,
    SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED,
    SOLIDSYSLOG_CAT_STORE_WRITE_FAILED,
    SOLIDSYSLOG_CAT_STORE_FROZEN,
    SOLIDSYSLOG_CAT_FILE_IO_FAILED,
    SOLIDSYSLOG_CAT_SENDER_RECORDS_LOST,
    SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED,
    SOLIDSYSLOG_CAT_SENDER_DELIVERY_RESTORED,
    SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
    SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED,
    SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED,
    SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
};

TEST(SolidSyslogPendingEvents, EveryReportedKindLandsInItsOwnSlot)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    for (size_t kind = 0U; kind < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT; kind++)
    {
        uint16_t category = REPORTED_KINDS[kind]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &source, category, (int32_t) kind);
    }

    for (size_t index = 0U; index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT; index++)
    {
        CHECK(SolidSyslogPendingEvents_Take(index, &taken));
        LONGS_EQUAL(REPORTED_KINDS[index], taken.Category); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        LONGS_EQUAL((int32_t) index, taken.Detail);
    }
}

TEST(SolidSyslogPendingEvents, TakeOfACategoryOutsideTheSetFindsNothingAndLeavesTheEventAlone)
{
    taken.Detail = 77;

    CHECK_FALSE(TakeKind(SOLIDSYSLOG_CAT_BAD_CONFIG));
    LONGS_EQUAL(77, taken.Detail);
}

TEST(SolidSyslogPendingEvents, TakeClearsTheSlotSoASecondTakeFindsNothing)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, &source, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, 7);

    CHECK(TakeKind(SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST));
    CHECK_FALSE(TakeKind(SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST));
}

static int detailNameCalls = 0;

static const char* RaisingDetailName(int32_t detail)
{
    static const struct SolidSyslogErrorSource self = {"raising", RaisingDetailName};
    (void) detail;
    detailNameCalls++;
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &self, SOLIDSYSLOG_CAT_STORE_FROZEN, 99);
    return "RAISED";
}

TEST(SolidSyslogPendingEvents, TheTapNeverNamesTheDetailSoARaisingDetailNameDoesNotRecurse)
{
    static const struct SolidSyslogErrorSource source = {"raising", RaisingDetailName};
    detailNameCalls = 0;
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &source, SOLIDSYSLOG_CAT_STORE_FROZEN, 1);
    LONGS_EQUAL(0, detailNameCalls);

    CHECK(TakeKind(SOLIDSYSLOG_CAT_STORE_FROZEN));
    STRCMP_EQUAL("RAISED", SolidSyslogErrorEvent_DetailName(&taken));

    LONGS_EQUAL(1, detailNameCalls);
    CHECK(TakeKind(SOLIDSYSLOG_CAT_STORE_FROZEN));
    LONGS_EQUAL(99, taken.Detail);
}

// clang-format off
TEST_GROUP(SolidSyslogPendingEventsInsideSend)
{
    struct SolidSyslogStream* stream = nullptr;
    struct SolidSyslogSender* sender = nullptr;
    struct SolidSyslogErrorEvent taken;
    int sentinel = 0;

    void setup() override
    {
        DrainEveryKind(&taken);
        ResolverFake_Reset();
        stream = StreamFake_Create();
        struct SolidSyslogStreamSenderConfig config = {};
        config.Resolver = ResolverFake_Get();
        config.Stream = stream;
        config.Address = AddressFake_Get();
        sender = SolidSyslogStreamSender_Create(&config);
        ErrorHandlerFake_Install(&sentinel);
    }

    void teardown() override
    {
        SolidSyslogStreamSender_Destroy(sender);
        StreamFake_Destroy(stream);
    }
};

// clang-format on

TEST(SolidSyslogPendingEventsInsideSend, ADeliveryFailureRaisedInsideSendIsCapturedWithoutRecursion)
{
    StreamFake_SetSendFails(stream, true);
    SolidSyslogSender_Send(sender, "x", 1U);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    CALLED_FAKE_ON(StreamFake_Send, stream, ONCE);
    CHECK(
        SolidSyslogPendingEvents_Take(SolidSyslogPendingEvents_IndexOf(SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED), &taken)
    );
    POINTERS_EQUAL(&SolidSyslogStreamSenderErrorSource, taken.Source);
}
