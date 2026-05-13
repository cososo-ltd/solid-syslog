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
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogStore.h"
}

#include <stdint.h>
#include <string.h>

#include <vector>

static const char* const TEST_PATH_PREFIX = "/tmp/draintest_";

struct DrainTestConfig
{
    /* cppcheck cannot follow CreateStore's reads through TEST_GROUP-
     * generated test classes; these fields ARE consumed there. */
    // cppcheck-suppress unusedStructMember
    size_t maxBlocks;
    // cppcheck-suppress unusedStructMember
    size_t maxBlockSize;
    size_t payloadSize;
    // cppcheck-suppress unusedStructMember
    enum SolidSyslogDiscardPolicy discardPolicy;
};

// clang-format off
TEST_GROUP(BlockStoreDrainOrdering)
{
    struct FileFakeStorage            fileStorage    = {};
    struct SolidSyslogFile*           file           = nullptr;
    SolidSyslogFileBlockDeviceStorage deviceStorage  = {};
    struct SolidSyslogBlockDevice*    device         = nullptr;
    SolidSyslogBlockStoreStorage      storeStorage   = {};
    struct SolidSyslogStore*          store          = nullptr;
    struct SolidSyslogSecurityPolicy* policy         = nullptr;

    void setup() override
    {
        file   = FileFake_Create(&fileStorage);
        device = SolidSyslogFileBlockDevice_Create(&deviceStorage, file, TEST_PATH_PREFIX);
        policy = SolidSyslogNullSecurityPolicy_Create();
    }

    void teardown() override
    {
        if (store != nullptr)
        {
            SolidSyslogBlockStore_Destroy(store);
        }
        SolidSyslogNullSecurityPolicy_Destroy();
        SolidSyslogFileBlockDevice_Destroy(device);
        FileFake_Destroy();
    }

    void CreateStore(const DrainTestConfig& cfg)
    {
        struct SolidSyslogBlockStoreConfig config = {};
        config.blockDevice                        = device;
        config.maxBlockSize                       = cfg.maxBlockSize;
        config.maxBlocks                          = cfg.maxBlocks;
        config.discardPolicy                      = cfg.discardPolicy;
        config.securityPolicy                     = policy;
        store                                     = SolidSyslogBlockStore_Create(&storeStorage, &config);
    }

    // Writes one record whose payload encodes sequenceId in the first 4
    // bytes (little-endian) and is padded with 'x' to the test's chosen
    // payload size. Returns the Write result so tests can pin discard
    // behaviour without asserting truthy here.
    bool WriteMessage(uint32_t sequenceId, size_t payloadSize)
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
    uint32_t DrainOne()
    {
        CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
        uint8_t buf[4096] = {};
        size_t  bytesRead = 0;
        CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
        SolidSyslogStore_MarkSent(store);
        return static_cast<uint32_t>(buf[0]) | (static_cast<uint32_t>(buf[1]) << 8) | (static_cast<uint32_t>(buf[2]) << 16) | (static_cast<uint32_t>(buf[3]) << 24);
    }

    std::vector<uint32_t> DrainAll()
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
    DrainTestConfig cfg = {/*maxBlocks=*/2, /*maxBlockSize=*/200, /*payloadSize=*/64, SOLIDSYSLOG_DISCARD_NEWEST};
    CreateStore(cfg);

    /* Pre-outage send + drain — mirrors `When the client sends a message`
     * + `Then the syslog oracle receives 1 message` in the BDD scenario. */
    CHECK_TRUE(WriteMessage(1, cfg.payloadSize));
    LONGS_EQUAL(1U, DrainOne());

    /* Outage period: 10 messages queued without intermediate drains. */
    for (uint32_t id = 2; id <= 11U; ++id)
    {
        (void) WriteMessage(id, cfg.payloadSize);
    }

    /* Drain everything that's still there. */
    std::vector<uint32_t> drained = DrainAll();

    /* Dump the drained sequence to stdout so we can see the shape even
     * when the structural assertion below passes. The user-visible
     * artifact from the BDD scenario is [1, 11, 2, 3, 4, 5, 6]; we need
     * to know what we actually produce here to compare. */
    (void) printf("[drain-ordering] outage drained %zu records: [", drained.size());
    for (size_t i = 0; i < drained.size(); ++i)
    {
        (void) printf("%s%u", (i == 0) ? "" : ", ", drained[i]);
    }
    (void) printf("]\n");

    /* Structural check: drain must be strictly ascending. Any descent
     * (e.g. 11 followed by 2) is an ordering bug regardless of which ids
     * the discard policy kept. */
    for (size_t i = 1; i < drained.size(); ++i)
    {
        if (drained[i] <= drained[i - 1])
        {
            char message[256];
            (void) snprintf(message, sizeof(message), "Drain order not strictly ascending: drained[%zu]=%u after drained[%zu]=%u", i, drained[i], i - 1,
                            drained[i - 1]);
            FAIL(message);
        }
    }
}
