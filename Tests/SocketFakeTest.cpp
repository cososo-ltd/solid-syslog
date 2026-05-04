#include <sys/socket.h>

#include "SocketFake.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(SocketFake)
{
    void setup() override { SocketFake_Reset(); }
};
// clang-format on

TEST(SocketFake, RecvIncrementsCount)
{
    char buf[16];
    recv(3, buf, sizeof(buf), 0);
    LONGS_EQUAL(1, SocketFake_RecvCallCount());
}

TEST(SocketFake, RecvCapturesFd)
{
    char buf[16];
    recv(7, buf, sizeof(buf), 0);
    LONGS_EQUAL(7, SocketFake_LastRecvFd());
}

TEST(SocketFake, RecvCapturesBuf)
{
    char buf[16];
    recv(3, buf, sizeof(buf), 0);
    POINTERS_EQUAL(buf, SocketFake_LastRecvBuf());
}

TEST(SocketFake, RecvCapturesLen)
{
    char buf[16];
    recv(3, buf, sizeof(buf), 0);
    LONGS_EQUAL(sizeof(buf), SocketFake_LastRecvLen());
}

TEST(SocketFake, RecvCapturesFlags)
{
    char buf[16];
    recv(3, buf, sizeof(buf), 42);
    LONGS_EQUAL(42, SocketFake_LastRecvFlags());
}
