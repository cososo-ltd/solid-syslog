#include <signal.h>

#include "CppUTest/CommandLineTestRunner.h"

int main(int argc, char* argv[])
{
    /* Negative-path handshake tests close the client fd while the server
     * thread may still be writing (e.g. its own alert in response to the
     * client's). Neither the stream's I/O callbacks nor SocketStream pass
     * MSG_NOSIGNAL, so a write to a closed peer would raise SIGPIPE and
     * abort the test process. Ignoring SIGPIPE turns the failure into an
     * EPIPE that wolfSSL and the callbacks surface as a normal transport
     * error. */
    signal(SIGPIPE, SIG_IGN);
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
