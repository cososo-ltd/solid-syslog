/* Integration-level harness over the real BlockStore + BlockSequence +
 * RecordStore + FileBlockDevice stack, with FileFake at the bottom.
 *
 * Motivated by S08.05's discard-newest BDD failure on freertos-cross
 * (#270): oracle received sequenceIds [1, 11, 2, 3, 4, 5, 6] — sequence
 * id 11 (the newest, which discard-newest should drop) appearing
 * mid-drain between 1 and 2 says the drain ordering is wrong, not the
 * record-packing math. This harness reproduces drain sequences host-side
 * so we can iterate in milliseconds and parameterise the size knobs
 * (maxBlocks / maxBlockSize / payload) without round-tripping QEMU.
 *
 * Tests drive the SolidSyslogStore interface (Write / HasUnsent /
 * ReadNextUnsent / MarkSent) directly — no Service task, no Sender, no
 * Buffer — because the drain order lives in BlockSequence/RecordStore. */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "FileFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogTunables.h"
}

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <stdint.h>

#include <vector>

static const char* const TEST_PATH_PREFIX = "/tmp/draintest_";

/* SenderSpy — sticky outage mode (every Send returns false until cleared)
 * and a vector of every *successful* send. Bigger than SenderFake's
 * last-only capture and one-shot FailNextSend; the BDD reproducer needs
 * to see the full successful-send sequence to spot the [1, 11, 2, ...]
 * interleave. */
struct SenderSpy
{
    struct SolidSyslogSender Base;
    std::vector<std::vector<uint8_t>> successfulSends;
    bool outage;
};

static bool SenderSpy_Send(struct SolidSyslogSender* self, const void* buffer, size_t size)
{
    auto* spy = reinterpret_cast<SenderSpy*>(self);
    if (spy->outage)
    {
        return false;
    }
    spy->successfulSends.emplace_back(static_cast<const uint8_t*>(buffer), static_cast<const uint8_t*>(buffer) + size);
    return true;
}

static void SenderSpy_Disconnect(struct SolidSyslogSender* self)
{
    (void) self;
}

static void SenderSpy_Init(SenderSpy& spy)
{
    spy.Base.Send = SenderSpy_Send;
    spy.Base.Disconnect = SenderSpy_Disconnect;
    spy.outage = false;
    spy.successfulSends.clear();
}

static uint32_t DecodeSequenceId(const std::vector<uint8_t>& payload)
{
    return static_cast<uint32_t>(payload[0]) | (static_cast<uint32_t>(payload[1]) << 8) |
           (static_cast<uint32_t>(payload[2]) << 16) | (static_cast<uint32_t>(payload[3]) << 24);
}

static std::vector<uint32_t> DecodeSequenceIds(const std::vector<std::vector<uint8_t>>& sends)
{
    std::vector<uint32_t> ids;
    ids.reserve(sends.size());
    std::transform(sends.begin(), sends.end(), std::back_inserter(ids), DecodeSequenceId);
    return ids;
}

struct DrainTestConfig
{
    /* cppcheck cannot follow CreateStore's reads through TEST_GROUP-
     * generated test classes; these fields ARE consumed there. */
    size_t MaxBlocks;
    size_t MaxBlockSize;
    size_t PayloadSize;
    enum SolidSyslogDiscardPolicy DiscardPolicy;
};

/* Shared block-device / null-security-policy fixture (the underlying FileFake is
 * an implementation detail of the BlockDevice). Lifted out of
 * the two TEST_GROUPs below so each can focus on its own moving parts —
 * BlockStoreDrainOrdering adds a BlockStore directly; ServiceDrainInterleave
 * adds Buffer + Mutex + SenderSpy + the SolidSyslog facade. Matches the
 * established TEST_BASE / TEST_GROUP_BASE pattern from
 * SolidSyslogBlockStoreTest.cpp. */
// clang-format off
TEST_BASE(DrainTestFixtureBase)
{
    struct FileFakeStorage            fileStorage = {};
    struct SolidSyslogFile*           file        = nullptr;
    struct SolidSyslogBlockDevice*    device      = nullptr;
    struct SolidSyslogSecurityPolicy* policy      = nullptr;

    void setupBlockDeviceAndPolicy()
    {
        file   = FileFake_Create(&fileStorage);
        device = SolidSyslogFileBlockDevice_Create(file, TEST_PATH_PREFIX, 4096);
        policy = SolidSyslogNullSecurityPolicy_Get();
    }

    /* Block size is a property of the device; re-point it (pool size 1, so
     * destroy-then-recreate on the same FileFake) at the scenario's size.
     * Idempotent — unchanged size reuses the existing device. */
    void ensureDeviceBlockSize(size_t blockSize)
    {
        if (SolidSyslogBlockDevice_GetBlockSize(device) != blockSize)
        {
            SolidSyslogFileBlockDevice_Destroy(device);
            device = SolidSyslogFileBlockDevice_Create(file, TEST_PATH_PREFIX, blockSize);
        }
    }

    void teardownBlockDeviceAndPolicy() const
    {
        SolidSyslogFileBlockDevice_Destroy(device);
        FileFake_Destroy();
    }
};

// clang-format on

// clang-format off
TEST_GROUP_BASE(BlockStoreDrainOrdering, DrainTestFixtureBase)
{
    struct SolidSyslogStore*          store          = nullptr;

    void setup() override
    {
        setupBlockDeviceAndPolicy();
    }

    void teardown() override
    {
        if (store != nullptr)
        {
            SolidSyslogBlockStore_Destroy(store);
        }
        teardownBlockDeviceAndPolicy();
    }

    void CreateStore(const DrainTestConfig& cfg)
    {
        ensureDeviceBlockSize(cfg.MaxBlockSize);
        struct SolidSyslogBlockStoreConfig config = {};
        config.BlockDevice                        = device;
        config.MaxBlocks                          = cfg.MaxBlocks;
        config.DiscardPolicy                      = cfg.DiscardPolicy;
        config.SecurityPolicy                     = policy;
        store                                     = SolidSyslogBlockStore_Create(&config);
    }

    // Writes one record whose payload encodes sequenceId in the first 4
    // bytes (little-endian) and is padded with 'x' to the test's chosen
    // payload size. Returns the Write result so tests can pin discard
    // behaviour without asserting truthy here.
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- sequenceId and payloadSize are distinct concepts; both numeric is incidental
    [[nodiscard]] bool WriteMessage(uint32_t sequenceId, size_t payloadSize) const
    {
        std::vector<uint8_t> buf(payloadSize, 'x');
        buf[0] = static_cast<uint8_t>(sequenceId & 0xFFU);
        buf[1] = static_cast<uint8_t>((sequenceId >> 8) & 0xFFU);
        buf[2] = static_cast<uint8_t>((sequenceId >> 16) & 0xFFU);
        buf[3] = static_cast<uint8_t>((sequenceId >> 24) & 0xFFU);
        return SolidSyslogStore_Write(store, buf.data(), buf.size());
    }

    // Drains the next unsent record. Returns the sequenceId decoded from
    // the first 4 bytes. Asserts there is something to drain so misuse
    // surfaces as a test failure, not a 0 quietly slipped into the list.
    [[nodiscard]] uint32_t DrainOne() const
    {
        CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
        uint8_t buf[4096] = {};
        size_t  bytesRead = 0;
        CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
        SolidSyslogStore_MarkSent(store);
        return static_cast<uint32_t>(buf[0]) | (static_cast<uint32_t>(buf[1]) << 8) | (static_cast<uint32_t>(buf[2]) << 16) | (static_cast<uint32_t>(buf[3]) << 24);
    }

    [[nodiscard]] std::vector<uint32_t> DrainAll() const
    {
        std::vector<uint32_t> drained;
        while (SolidSyslogStore_HasUnsent(store))
        {
            drained.push_back(DrainOne());
        }
        return drained;
    }
};

// clang-format on

/* Service-level reproducer — wires the real Buffer drain logic from
 * SolidSyslog_Service against a real BlockStore and a SenderSpy that
 * simulates oracle outage / recovery. This is the harness where the
 * [1, 11, 2, 3, ...] BDD shape actually arises — DrainBufferIntoStore
 * falls back to Sender_Send when Store_Write rejects (NullStore path),
 * which on a full BlockStore in discard-newest mode lets the *latest*
 * buffered message bypass *older* stored messages once the oracle
 * recovers. */
// clang-format off
TEST_GROUP_BASE(ServiceDrainInterleave, DrainTestFixtureBase)
{
    /* Sized to hold 16 max-sized messages — plenty for the outage
     * reproducer; CircularBuffer is FIFO so all messages are retained
     * until Service drains them (unlike BufferFake which only keeps
     * the last one). */
    uint8_t                          bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(16)] = {};
    struct SolidSyslog*              solidSyslog                                               = nullptr;
    struct SolidSyslogStore*         store                                                     = nullptr;
    struct SolidSyslogMutex*         mutex                                                     = nullptr;
    struct SolidSyslogBuffer*        buffer                                                    = nullptr;
    SenderSpy                        spy                                                       = {};

    void setup() override
    {
        setupBlockDeviceAndPolicy();
        mutex  = SolidSyslogNullMutex_Get();
        buffer = SolidSyslogCircularBuffer_Create(mutex, bufferRing, sizeof(bufferRing));
        SenderSpy_Init(spy);
    }

    void teardown() override
    {
        SolidSyslog_Destroy(solidSyslog);
        solidSyslog = nullptr;
        if (store != nullptr)
        {
            SolidSyslogBlockStore_Destroy(store);
            store = nullptr;
        }
        SolidSyslogCircularBuffer_Destroy(buffer);
        teardownBlockDeviceAndPolicy();
    }

    /* Build BlockStore + wire SolidSyslog facade with buffer + store + spy. */
    void Setup(const DrainTestConfig& cfg)
    {
        ensureDeviceBlockSize(cfg.MaxBlockSize);
        struct SolidSyslogBlockStoreConfig storeCfg = {};
        storeCfg.BlockDevice                        = device;
        storeCfg.MaxBlocks                          = cfg.MaxBlocks;
        storeCfg.DiscardPolicy                      = cfg.DiscardPolicy;
        storeCfg.SecurityPolicy                     = policy;
        store                                       = SolidSyslogBlockStore_Create(&storeCfg);

        struct SolidSyslogConfig sysCfg = {};
        sysCfg.Buffer                   = buffer;
        sysCfg.Sender                   = &spy.Base;
        sysCfg.Store                    = store;
        solidSyslog = SolidSyslog_Create(&sysCfg);
    }

    /* Push one record into the buffer with sequenceId encoded in the first
     * 4 bytes — bypasses SolidSyslog_Log so we control exact bytes and
     * don't pull in clock / hostname / SD plumbing. */
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- sequenceId and payloadSize are distinct concepts; both numeric is incidental
    void Enqueue(uint32_t sequenceId, size_t payloadSize) const
    {
        std::vector<uint8_t> buf(payloadSize, 'x');
        buf[0] = static_cast<uint8_t>(sequenceId & 0xFFU);
        buf[1] = static_cast<uint8_t>((sequenceId >> 8) & 0xFFU);
        buf[2] = static_cast<uint8_t>((sequenceId >> 16) & 0xFFU);
        buf[3] = static_cast<uint8_t>((sequenceId >> 24) & 0xFFU);
        SolidSyslogBuffer_Write(buffer, buf.data(), buf.size());
    }

    void ServiceTickUntilQuiet(size_t cap) const
    {
        for (size_t i = 0; i < cap; ++i)
        {
            SolidSyslog_Service(solidSyslog);
        }
    }
};

// clang-format on

TEST(ServiceDrainInterleave, DiscardNewestDoesNotLetNewestBypassOldestOnRecovery)
{
    /* Size the payload to MAX - small-slack so the runtime clamp on
     * maxBlockSize bottoms out at ~MAX+overhead and each block holds
     * exactly one record. MAX-relative keeps the test portable across
     * SOLIDSYSLOG_MAX_MESSAGE_SIZE tunable overrides. With maxBlocks=2
     * the store fits 2 records — small enough for the outage to overflow
     * with just a couple of messages. */
    DrainTestConfig cfg = {
        /*maxBlocks=*/2,
        /*maxBlockSize=*/200 /*grown to one record by the store*/,
        /*payloadSize=*/SOLIDSYSLOG_MAX_MESSAGE_SIZE - 100U,
        SOLIDSYSLOG_DISCARD_POLICY_NEWEST
    };
    Setup(cfg);

    /* Pre-outage send: msg 1 flows buffer -> store -> sender successfully. */
    Enqueue(1, cfg.PayloadSize);
    SolidSyslog_Service(solidSyslog);
    LONGS_EQUAL(1U, spy.successfulSends.size());

    /* Outage begins. */
    spy.outage = true;

    /* Two messages fit into the 2-block store. */
    Enqueue(2, cfg.PayloadSize);
    Enqueue(3, cfg.PayloadSize);
    SolidSyslog_Service(solidSyslog);

    /* Message 11 arrives still in outage — it lands in the buffer but
     * hasn't been pulled by Service yet at the moment the oracle resumes. */
    Enqueue(11, cfg.PayloadSize);

    /* Oracle resumes. Drain by ticking Service repeatedly. */
    spy.outage = false;
    ServiceTickUntilQuiet(10);
    CHECK_FALSE_TEXT(
        SolidSyslogStore_HasUnsent(store),
        "store still holds unsent records after recovery — ServiceTickUntilQuiet cap too small"
    );
    CHECK_TRUE_TEXT(
        spy.successfulSends.size() > 1U,
        "expected post-recovery successful sends; only the pre-outage send fired"
    );

    std::vector<uint32_t> ids = DecodeSequenceIds(spy.successfulSends);

    /* Structural assertion: successful sends must be in non-descending
     * order. ANY descent (e.g. 11 followed by 2) means a newer message
     * jumped ahead of older ones — exactly the bug the BDD scenario
     * pins. */
    for (size_t i = 1; i < ids.size(); ++i)
    {
        if (ids[i] < ids[i - 1])
        {
            char message[256] = {};
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) -- snprintf is the lingua franca for building CppUTest FAIL messages
            (void) snprintf(
                message,
                sizeof(message),
                "Send order descended: ids[%zu]=%u after ids[%zu]=%u",
                i,
                ids[i],
                i - 1,
                ids[i - 1]
            );
            FAIL(message);
        }
    }
}

/* Reproducer for the BDD discard-newest failure shape. With max-blocks=2
 * and a small max-block-size relative to the payload, an "outage" of 10
 * messages should produce *some* drain order — what matters is that the
 * ids drained are strictly ascending (oldest-first), regardless of which
 * ones got discarded by the policy.
 *
 * BDD on freertos-cross saw [1, 11, 2, 3, 4, 5, 6] — sequenceId 11
 * interleaved between 1 and 2, which is structurally impossible for a
 * correct oldest-first drain. If this test reproduces that interleave,
 * we have the bug in our hands. */
TEST(BlockStoreDrainOrdering, OutageDrainProducesAscendingSequenceIds)
{
    DrainTestConfig cfg =
        {/*maxBlocks=*/2, /*maxBlockSize=*/200, /*payloadSize=*/64, SOLIDSYSLOG_DISCARD_POLICY_NEWEST};
    CreateStore(cfg);

    /* Pre-outage send + drain — mirrors `When the client sends a message`
     * + `Then the syslog oracle receives 1 message` in the BDD scenario. */
    CHECK_TRUE(WriteMessage(1, cfg.PayloadSize));
    LONGS_EQUAL(1U, DrainOne());

    /* Outage period: 10 messages queued without intermediate drains. */
    for (uint32_t id = 2; id <= 11U; ++id)
    {
        (void) WriteMessage(id, cfg.PayloadSize);
    }

    /* Drain everything that's still there. */
    std::vector<uint32_t> drained = DrainAll();

    /* Structural check: drain must be strictly ascending. Any descent
     * (e.g. 11 followed by 2) is an ordering bug regardless of which ids
     * the discard policy kept. */
    for (size_t i = 1; i < drained.size(); ++i)
    {
        if (drained[i] <= drained[i - 1])
        {
            char message[256] = {};
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) -- snprintf is the lingua franca for building CppUTest FAIL messages
            (void) snprintf(
                message,
                sizeof(message),
                "Drain order not strictly ascending: drained[%zu]=%u after drained[%zu]=%u",
                i,
                drained[i],
                i - 1,
                drained[i - 1]
            );
            FAIL(message);
        }
    }
}
