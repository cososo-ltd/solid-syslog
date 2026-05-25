#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "MqFake.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros

static mqd_t OpenTestQueue(const char* name, long maxMessages = 4, size_t maxMessageSize = 64)
{
    struct mq_attr attr = {0, maxMessages, static_cast<long>(maxMessageSize), 0, {0}};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) -- POSIX mq_open is a varargs function when O_CREAT is set
    return mq_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);
}

// clang-format off
TEST_GROUP(MqFake)
{
    void setup() override { MqFake_Reset(); }
};
// clang-format on

TEST(MqFake, OpenIncrementsCount)
{
    OpenTestQueue("/test");
    CALLED_FAKE(MqFake_Open, ONCE);
}

TEST(MqFake, OpenCapturesName)
{
    OpenTestQueue("/captured");
    STRCMP_EQUAL("/captured", MqFake_LastOpenName());
}

TEST(MqFake, OpenCapturesAttrFields)
{
    OpenTestQueue("/test", 7, 128);
    LONGS_EQUAL(7, MqFake_LastOpenMaxMessages());
    LONGS_EQUAL(128, MqFake_LastOpenMaxMessageSize());
}

TEST(MqFake, OpenReturnsDistinctMqdPerCall)
{
    mqd_t first = OpenTestQueue("/first");
    mqd_t second = OpenTestQueue("/second");
    CHECK(first != second);
    CHECK(first != (mqd_t) -1);
    CHECK(second != (mqd_t) -1);
}

TEST(MqFake, FailNextOpenReturnsMinusOneAndSetsErrno)
{
    MqFake_FailNextOpen(EINVAL);
    mqd_t result = OpenTestQueue("/test");
    LONGS_EQUAL(-1, (long) result);
    LONGS_EQUAL(EINVAL, errno);
}

TEST(MqFake, FailNextOpenIsOneShot)
{
    MqFake_FailNextOpen(EINVAL);
    (void) OpenTestQueue("/first");
    mqd_t second = OpenTestQueue("/second");
    CHECK(second != (mqd_t) -1);
}

TEST(MqFake, OpenNameHistoryReturnsNameByIndex)
{
    OpenTestQueue("/alpha");
    OpenTestQueue("/beta");
    STRCMP_EQUAL("/alpha", MqFake_OpenNameAt(0));
    STRCMP_EQUAL("/beta", MqFake_OpenNameAt(1));
}

TEST(MqFake, SendStoresMessageForReceive)
{
    mqd_t mqd = OpenTestQueue("/test");
    LONGS_EQUAL(0, mq_send(mqd, "payload", 7, 0));

    char buf[16] = {};
    ssize_t received = mq_receive(mqd, buf, sizeof(buf), nullptr);
    LONGS_EQUAL(7, received);
    STRCMP_EQUAL("payload", buf);
}

TEST(MqFake, ReceiveFromEmptyQueueReturnsMinusOneEagain)
{
    mqd_t mqd = OpenTestQueue("/test");

    char buf[16] = {};
    ssize_t result = mq_receive(mqd, buf, sizeof(buf), nullptr);
    LONGS_EQUAL(-1, result);
    LONGS_EQUAL(EAGAIN, errno);
}

TEST(MqFake, FailNextSendReturnsMinusOneAndSetsErrno)
{
    mqd_t mqd = OpenTestQueue("/test");
    MqFake_FailNextSend(EMSGSIZE);
    LONGS_EQUAL(-1, mq_send(mqd, "x", 1, 0));
    LONGS_EQUAL(EMSGSIZE, errno);
}

/* Natural overflow — sending past mq_attr.mq_maxmsg must surface as
 * -1/EAGAIN per POSIX, not silently succeed. */
TEST(MqFake, SendBeyondMaxMessagesReturnsMinusOneEagain)
{
    mqd_t mqd = OpenTestQueue("/test", /*maxMessages=*/2, /*maxMessageSize=*/64);
    LONGS_EQUAL(0, mq_send(mqd, "a", 1, 0));
    LONGS_EQUAL(0, mq_send(mqd, "b", 1, 0));

    LONGS_EQUAL(-1, mq_send(mqd, "c", 1, 0));
    LONGS_EQUAL(EAGAIN, errno);
}

/* Natural oversize — POSIX requires -1/EMSGSIZE, not truncation. */
TEST(MqFake, SendLargerThanMaxMessageSizeReturnsMinusOneEmsgsize)
{
    mqd_t mqd = OpenTestQueue("/test", /*maxMessages=*/4, /*maxMessageSize=*/4);
    LONGS_EQUAL(-1, mq_send(mqd, "fivefive", 8, 0));
    LONGS_EQUAL(EMSGSIZE, errno);
}

TEST(MqFake, FailNextReceiveReturnsMinusOneAndSetsErrno)
{
    mqd_t mqd = OpenTestQueue("/test");
    mq_send(mqd, "x", 1, 0);
    MqFake_FailNextReceive(EBADMSG);
    char buf[16] = {};
    LONGS_EQUAL(-1, mq_receive(mqd, buf, sizeof(buf), nullptr));
    LONGS_EQUAL(EBADMSG, errno);
}

TEST(MqFake, CloseIncrementsCount)
{
    mqd_t mqd = OpenTestQueue("/test");
    mq_close(mqd);
    CALLED_FAKE(MqFake_Close, ONCE);
    LONGS_EQUAL((long) mqd, (long) MqFake_LastClosedMqd());
}

TEST(MqFake, UnlinkIncrementsCountAndCapturesName)
{
    mq_unlink("/gone");
    CALLED_FAKE(MqFake_Unlink, ONCE);
    STRCMP_EQUAL("/gone", MqFake_LastUnlinkName());
}

TEST(MqFake, IsolatedQueuesDoNotShareMessages)
{
    mqd_t alpha = OpenTestQueue("/alpha");
    mqd_t beta = OpenTestQueue("/beta");

    mq_send(alpha, "from-alpha", 10, 0);

    char buf[16] = {};
    ssize_t result = mq_receive(beta, buf, sizeof(buf), nullptr);
    LONGS_EQUAL(-1, result);
    LONGS_EQUAL(EAGAIN, errno);
}
