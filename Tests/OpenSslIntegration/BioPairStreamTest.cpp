/* The Stream contract obliges an implementation to close internally on a
 * failed Send and before a negative Read, so the caller can reopen and
 * store-and-forward can replay. BioPairStream sits underneath the OpenSSL
 * contract tests, so a double more forgiving than the contract would hide a
 * reconnect path that misbehaves against a conforming peer - #727.
 *
 * Closing cannot mean freeing the BIO here: the pair's lifetime belongs to
 * the harness, which keeps pumping the peer. What it means instead is that
 * the stream refuses to carry anything until it is reopened, which is the
 * part of the clause a caller can observe.
 *
 * Every test drains or fills the peer side before asserting. A pair left full
 * refuses a write, and a pair left empty reads as a would-block, so an
 * assertion made without draining passes whether the stream closed or not. */
#include <openssl/bio.h>

#include "BioPairStream.h"
#include "SolidSyslogStream.h"

#include "CppUTest/TestHarness.h"

namespace
{
constexpr int PAIR_BYTES = 16;
constexpr size_t OVERSIZE_WRITE = 64;
} // namespace

// clang-format off
TEST_GROUP(BioPairStreamContract)
{
    BIO* ours = nullptr;
    BIO* theirs = nullptr;
    struct SolidSyslogStream* stream = nullptr;
    char payload[OVERSIZE_WRITE] = {};

    void setup() override
    {
        CHECK(BIO_new_bio_pair(&ours, PAIR_BYTES, &theirs, PAIR_BYTES) == 1);
        stream = BioPairStream_Create(ours);
    }

    void teardown() override
    {
        BioPairStream_Destroy(stream);
        BIO_free(ours);
        BIO_free(theirs);
    }

    /** A write larger than the pair cannot go in full, which is the failure
     *  the contract is about - a partial write is never a success. */
    void sendFails() const
    {
        CHECK_FALSE(SolidSyslogStream_Send(stream, payload, OVERSIZE_WRITE));
    }

    /** Empty the peer side so the pair has room again. Without this a later
     *  write fails because the pair is full rather than because the stream
     *  closed, and the assertion proves nothing. */
    void drainPeer() const
    {
        char sink[PAIR_BYTES] = {};
        while (BIO_read(theirs, sink, sizeof(sink)) > 0)
        {
        }
    }

    /** Give the stream something to read, so a negative return is the stream
     *  refusing rather than the pair being empty. */
    void peerSends() const
    {
        CHECK(BIO_write(theirs, "y", 1) == 1);
    }
};

// clang-format on

TEST(BioPairStreamContract, SendThatCannotCompleteInFullFails)
{
    sendFails();
}

TEST(BioPairStreamContract, SendThatCannotCompleteClosesTheStream)
{
    sendFails();
    drainPeer();
    CHECK_FALSE(SolidSyslogStream_Send(stream, payload, 1));
}

TEST(BioPairStreamContract, AClosedStreamReadsAsTornDownRatherThanIdle)
{
    char buffer[1] = {};
    peerSends();
    SolidSyslogStream_Close(stream);
    CHECK(SolidSyslogStream_Read(stream, buffer, sizeof(buffer)) < 0);
}

TEST(BioPairStreamContract, ReopeningRestoresTheStream)
{
    sendFails();
    drainPeer();
    CHECK(SolidSyslogStream_Open(stream, nullptr));
    CHECK(SolidSyslogStream_Send(stream, payload, 1));
}
