/* The Stream contract obliges an implementation to close internally on a
 * failed Send and before a negative Read, so the caller can reopen and
 * store-and-forward can replay. SocketStream sits underneath the Mbed TLS
 * contract tests, so a double more forgiving than the contract would hide a
 * reconnect path that misbehaves against a conforming peer - #727.
 *
 * A file descriptor that is open but is not a socket makes send and recv fail
 * deterministically with ENOTSOCK, and raises no SIGPIPE. That the double
 * closed is observed on the descriptor itself: fcntl answers EBADF once it
 * has gone. */
#include <fcntl.h>
#include <unistd.h>

#include "SocketStream.h"
#include "SolidSyslogStream.h"

#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(SocketStreamContract)
{
    struct SolidSyslogStream* stream = nullptr;
    int fd = -1;

    void setup() override
    {
        fd = open("/dev/null", O_RDWR);
        CHECK(fd >= 0);
        stream = SocketStream_Create(fd);
    }

    void teardown() override
    {
        SocketStream_Destroy(stream);
    }

    [[nodiscard]] bool descriptorIsOpen() const
    {
        return fcntl(fd, F_GETFD) != -1;
    }
};

// clang-format on

TEST(SocketStreamContract, SendThatCannotCompleteClosesTheStream)
{
    CHECK_FALSE(SolidSyslogStream_Send(stream, "x", 1));
    CHECK_FALSE(descriptorIsOpen());
}

TEST(SocketStreamContract, ReadThatFailsClosesTheStreamBeforeReturningNegative)
{
    char buffer[1] = {};
    CHECK(SolidSyslogStream_Read(stream, buffer, sizeof(buffer)) < 0);
    CHECK_FALSE(descriptorIsOpen());
}
