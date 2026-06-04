#include <stdint.h>
#include <stdio.h>
#include <cstring>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogBlockStoreErrors.h"
#include "SolidSyslogCrc16Policy.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogTunables.h"
#include "ErrorHandlerFake.h"
#include "FileFake.h"
#include "TestUtils.h"

using namespace CososoTesting;

static const char* const TEST_PATH_PREFIX = "/tmp/test_store";
static const char* const TEST_DATA = "hello";
static const size_t TEST_DATA_LEN = 5;

enum
{
    TEST_BUF_SIZE = SOLIDSYSLOG_MAX_MESSAGE_SIZE,
    SENTINEL = 'Z',
    /* Mirrors the private RECORD_OVERHEAD in SolidSyslogBlockStore.c:
     * MAGIC_SIZE(2) + RECORD_LENGTH_SIZE(2) + SENT_FLAG_SIZE(1). */
    TEST_RECORD_OVERHEAD = 5,
    TEST_RECORDS_PER_BLOCK = 2,
    /* Sized to fit TEST_RECORDS_PER_BLOCK worst-case records — the worst
     * case being max-size data plus max-integrity bytes. Auto-adapts
     * when SOLIDSYSLOG_MAX_MESSAGE_SIZE or the integrity policy bound
     * are tuned. */
    TEST_MAX_BLOCK_SIZE =
        TEST_RECORDS_PER_BLOCK * (SOLIDSYSLOG_MAX_MESSAGE_SIZE + TEST_RECORD_OVERHEAD + SOLIDSYSLOG_MAX_INTEGRITY_SIZE),
    TEST_MAX_BLOCKS = 2
};

static const struct SolidSyslogBlockStoreConfig DEFAULT_CONFIG = {
    nullptr,
    TEST_MAX_BLOCKS,
    SOLIDSYSLOG_DISCARD_POLICY_OLDEST,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

static struct SolidSyslogBlockStoreConfig MakeConfig(struct SolidSyslogBlockDevice* device)
{
    struct SolidSyslogBlockStoreConfig config = DEFAULT_CONFIG;
    config.BlockDevice = device;
    return config;
}

/* Shared fixture — every BlockStore test group needs one FileFake backing the
 * BlockDevice, the BlockDevice itself, and a teardown that closes them in the
 * right order. TEST_BASE / TEST_GROUP_BASE lifts that boilerplate out of every
 * group. Test bodies still reference `file`, `device` directly because they
 * are inherited members. `file` is genuinely a `SolidSyslogFile*` (used for
 * file-layer assertions like `SolidSyslogFile_Exists` and fault injection
 * like `FileFake_FailNextOpen`); the BlockStore concept itself is exercised
 * through `device` and `store`. */
// clang-format off
TEST_BASE(BlockDeviceTestBase)
{
    struct FileFakeStorage storage = {};
    struct SolidSyslogFile* file = nullptr;
    struct SolidSyslogBlockDevice* device = nullptr;

    void setupBlockDeviceFakes()
    {
        file   = FileFake_Create(&storage);
        device = SolidSyslogFileBlockDevice_Create(file, TEST_PATH_PREFIX, TEST_MAX_BLOCK_SIZE);
    }

    void teardownBlockDeviceFakes() const
    {
        if (device != nullptr)
        {
            SolidSyslogFileBlockDevice_Destroy(device);
        }
        FileFake_Destroy();
    }

    /* Block size is now a property of the device, not the store config. Tests that
     * exercise a specific block size re-point the fixture device at it (pool size 1,
     * so destroy-then-recreate on the same FileFake, mirroring CreateWithPathPrefix).
     * Idempotent: when the size is unchanged the existing device is reused, so tests
     * that rebuild the store on the same device (e.g. corruption recovery) keep their
     * single persistent file handle. */
    void EnsureDeviceBlockSize(size_t blockSize)
    {
        if (SolidSyslogBlockDevice_GetBlockSize(device) != blockSize)
        {
            SolidSyslogFileBlockDevice_Destroy(device);
            device = SolidSyslogFileBlockDevice_Create(file, TEST_PATH_PREFIX, blockSize);
        }
    }
};

// clang-format on

/* ------------------------------------------------------------------
 * Basic operations
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP_BASE(SolidSyslogBlockStore, BlockDeviceTestBase)
{
    struct SolidSyslogStore* store = nullptr;

    void setup() override
    {
        setupBlockDeviceFakes();
        struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
        store = SolidSyslogBlockStore_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogBlockStore_Destroy(store);
        teardownBlockDeviceFakes();
    }
};

// clang-format on

TEST(SolidSyslogBlockStore, CreateReturnsNonNull)
{
    CHECK_TRUE(store != nullptr);
}

TEST(SolidSyslogBlockStore, CreatesFirstBlockOnInit)
{
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));
}

TEST(SolidSyslogBlockStore, HasUnsentReturnsFalseOnEmpty)
{
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStore, WriteReturnsTrue)
{
    CHECK_TRUE(SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN));
}

TEST(SolidSyslogBlockStore, HasUnsentReturnsTrueAfterWrite)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStore, ReadNextUnsentReturnsTrueAfterWrite)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
}

TEST(SolidSyslogBlockStore, ReadNextUnsentReturnsWrittenData)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char buf[TEST_BUF_SIZE] = {};
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL(TEST_DATA, buf, TEST_DATA_LEN);
}

TEST(SolidSyslogBlockStore, ReadNextUnsentReturnsByteCount)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    LONGS_EQUAL(TEST_DATA_LEN, bytesRead);
}

TEST(SolidSyslogBlockStore, ReadNextUnsentReturnsFalseOnEmpty)
{
    char buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    CHECK_FALSE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
}

TEST(SolidSyslogBlockStore, ReadNextUnsentSetsZeroBytesOnEmpty)
{
    char buf[TEST_BUF_SIZE];
    size_t bytesRead = 99;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    LONGS_EQUAL(0, bytesRead);
}

TEST(SolidSyslogBlockStore, ReadDoesNotWriteBeyondDataLength)
{
    char buf[TEST_BUF_SIZE];
    memset(buf, SENTINEL, sizeof(buf));

    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);

    MEMCMP_EQUAL(TEST_DATA, buf, TEST_DATA_LEN);
    BYTES_EQUAL(SENTINEL, buf[TEST_DATA_LEN]);
}

TEST(SolidSyslogBlockStore, ReadTruncatesWhenBufferTooSmall)
{
    const char* longMessage = "hello world";

    enum
    {
        SMALL_BUF_SIZE = 5
    };

    SolidSyslogStore_Write(store, longMessage, strlen(longMessage));

    char buf[SMALL_BUF_SIZE] = {};
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);

    LONGS_EQUAL(SMALL_BUF_SIZE, bytesRead);
    MEMCMP_EQUAL("hello", buf, SMALL_BUF_SIZE);
}

TEST(SolidSyslogBlockStore, MarkSentThenHasUnsentReturnsFalse)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStore, MarkSentWithoutReadDoesNotCrash)
{
    SolidSyslogStore_MarkSent(store);
}

TEST(SolidSyslogBlockStore, HasUnsentFalseAfterAllSent)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStore, WriteAfterDrainWorks)
{
    SolidSyslogStore_Write(store, "first", strlen("first"));
    char buf[TEST_BUF_SIZE] = {};
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    SolidSyslogStore_Write(store, "second", strlen("second"));
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("second", buf, strlen("second"));
}

TEST(SolidSyslogBlockStore, TwoWritesFirstReadReturnsFirst)
{
    SolidSyslogStore_Write(store, "first", strlen("first"));
    SolidSyslogStore_Write(store, "second", strlen("second"));
    char buf[TEST_BUF_SIZE] = {};
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("first", buf, strlen("first"));
}

TEST(SolidSyslogBlockStore, AfterMarkFirstReadReturnsSecond)
{
    SolidSyslogStore_Write(store, "first", strlen("first"));
    SolidSyslogStore_Write(store, "second", strlen("second"));
    char buf[TEST_BUF_SIZE] = {};
    size_t bytesRead = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("second", buf, strlen("second"));
    LONGS_EQUAL(strlen("second"), bytesRead);
}

TEST(SolidSyslogBlockStore, FiveWritesDrainAllInOrder)
{
    const char* messages[] = {"msg0", "msg1", "msg2", "msg3", "msg4"};

    enum
    {
        MESSAGE_LEN = 4
    };

    for (const auto* msg : messages)
    {
        SolidSyslogStore_Write(store, msg, MESSAGE_LEN);
    }

    for (const auto* expected : messages)
    {
        char buf[TEST_BUF_SIZE] = {};
        size_t bytesRead = 0;
        CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
        MEMCMP_EQUAL(expected, buf, MESSAGE_LEN);
        SolidSyslogStore_MarkSent(store);
    }

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

/* ------------------------------------------------------------------
 * Resume from existing block
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP_BASE(SolidSyslogBlockStoreResume, BlockDeviceTestBase)
{
    struct SolidSyslogStore* store = nullptr;

    void setup() override
    {
        setupBlockDeviceFakes();
    }

    void teardown() override
    {
        SolidSyslogBlockStore_Destroy(store);
        teardownBlockDeviceFakes();
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- total and markedSent have distinct semantics
    void WritePreviousSession(int total, int markedSent)
    {
        struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
        store = SolidSyslogBlockStore_Create(&config);
        WriteMessages(total);
        DrainMessages(markedSent);
        SolidSyslogBlockStore_Destroy(store);

        store = SolidSyslogBlockStore_Create(&config);
    }

    void WriteMessages(int count) const
    {
        for (int i = 0; i < count; i++)
        {
            char msg[16];
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) -- snprintf is the bounded C formatting API
            snprintf(msg, sizeof(msg), "msg%d", i);
            SolidSyslogStore_Write(store, msg, strlen(msg));
        }
    }

    void DrainMessages(int count) const
    {
        char   buf[TEST_BUF_SIZE];
        size_t bytesRead = 0;
        for (int i = 0; i < count; i++)
        {
            SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
            SolidSyslogStore_MarkSent(store);
        }
    }
};

// clang-format on

TEST(SolidSyslogBlockStoreResume, HasUnsentAfterResume)
{
    WritePreviousSession(3, 1);
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreResume, ReadReturnsFirstUnsent)
{
    WritePreviousSession(3, 1);
    char buf[TEST_BUF_SIZE] = {};
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("msg1", buf, strlen("msg1"));
}

TEST(SolidSyslogBlockStoreResume, DrainsRemainingUnsent)
{
    WritePreviousSession(3, 1);
    char buf[TEST_BUF_SIZE] = {};
    size_t bytesRead = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("msg1", buf, strlen("msg1"));
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("msg2", buf, strlen("msg2"));
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreResume, AllSentReturnsNoUnsent)
{
    WritePreviousSession(3, 3);
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreResume, EmptyPreviousSessionReturnsNoUnsent)
{
    WritePreviousSession(0, 0);
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreResume, CanWriteNewMessagesAfterResume)
{
    WritePreviousSession(2, 1);
    char buf[TEST_BUF_SIZE] = {};
    size_t bytesRead = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    SolidSyslogStore_Write(store, "new", strlen("new"));
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("new", buf, strlen("new"));
}

/* ------------------------------------------------------------------
 * Destroy
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP_BASE(SolidSyslogBlockStoreDestroy, BlockDeviceTestBase)
{
    void setup() override
    {
        setupBlockDeviceFakes();
    }

    void teardown() override
    {
        teardownBlockDeviceFakes();
    }
};

// clang-format on

TEST(SolidSyslogBlockStoreDestroy, DoubleDestroyDoesNotCrash)
{
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    struct SolidSyslogStore* store = SolidSyslogBlockStore_Create(&config);
    SolidSyslogBlockStore_Destroy(store);
    SolidSyslogBlockStore_Destroy(store);
}

/* ------------------------------------------------------------------
 * Config validation
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP_BASE(SolidSyslogBlockStoreConfig, BlockDeviceTestBase)
{
    struct SolidSyslogStore*   store   = nullptr;

    void setup() override
    {
        setupBlockDeviceFakes();
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
        SolidSyslogBlockStore_Destroy(store);
        teardownBlockDeviceFakes();
    }

    void CreateWithMaxBlocks(size_t maxBlocks)
    {
        struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
        config.MaxBlocks = maxBlocks;
        store = SolidSyslogBlockStore_Create(&config);
    }

    void CreateWithMaxBlockSize(size_t maxBlockSize)
    {
        EnsureDeviceBlockSize(maxBlockSize);
        struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
        store = SolidSyslogBlockStore_Create(&config);
    }

    void CreateWithPathPrefix(const char* prefix)
    {
        SolidSyslogFileBlockDevice_Destroy(device);
        device = SolidSyslogFileBlockDevice_Create(file, prefix, 4096);
        struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
        store = SolidSyslogBlockStore_Create(&config);
    }

    void VerifyWriteAndReadBack() const
    {
        CHECK_TRUE(SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN));
        char   buf[TEST_BUF_SIZE] = {};
        size_t bytesRead = 0;
        SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
        MEMCMP_EQUAL(TEST_DATA, buf, TEST_DATA_LEN);
    }
};

// clang-format on

TEST(SolidSyslogBlockStoreConfig, MaxBlocksZeroClampedToMinimum)
{
    CreateWithMaxBlocks(0);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogBlockStoreConfig, MaxBlocksOneClampedToMinimum)
{
    CreateWithMaxBlocks(1);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogBlockStoreConfig, MaxBlocksHundredClampedToMaximum)
{
    CreateWithMaxBlocks(100);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogBlockStoreConfig, ZeroBlockSizeUsesDeviceDefault)
{
    CreateWithMaxBlockSize(0);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogBlockStoreConfig, BelowMinimumBlockSizeIsGrownAndStoreWorks)
{
    CreateWithMaxBlockSize(1);
    CHECK(store != SolidSyslogNullStore_Get());
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogBlockStoreConfig, BelowMinimumBlockSizeReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    CreateWithMaxBlockSize(1);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_CONFIG, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(BLOCKSTORE_ERROR_BLOCK_TOO_SMALL, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogBlockStoreConfig, FilenameExactlyAtMaxPath)
{
    /* SOLIDSYSLOG_MAX_PATH_SIZE=128, suffix "00.log"=6, null=1, so max prefix=121 chars */
    char prefix[122];
    memset(prefix, 'A', 121);
    prefix[121] = '\0';

    CreateWithPathPrefix(prefix);

    char expected[129];
    memset(expected, 'A', 121);
    memcpy(expected + 121, "00.log", 7);

    CHECK_TRUE(SolidSyslogFile_Exists(file, expected));
}

TEST(SolidSyslogBlockStoreConfig, FilenameTruncatedWhenPrefixTooLong)
{
    /* SOLIDSYSLOG_MAX_PATH_SIZE=128. A 127-char prefix leaves 1 byte for digits and
       suffix. FormatFilename must not write past the buffer — prior to
       the fix, SolidSyslogFormat_Character wrote 2 bytes unconditionally
       (char + null), overflowing filename[128]. ASan detects this. */
    char prefix[128];
    memset(prefix, 'B', 127);
    prefix[127] = '\0';

    CreateWithPathPrefix(prefix);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogBlockStoreConfig, NullSecurityPolicyDefaultsToNoOp)
{
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    config.SecurityPolicy = nullptr;
    store = SolidSyslogBlockStore_Create(&config);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogBlockStoreConfig, OversizedSecurityPolicyLeavesNoIntegrityGap)
{
    struct SolidSyslogSecurityPolicy oversizedPolicy = {
        SOLIDSYSLOG_MAX_INTEGRITY_SIZE + 1,
        nullptr,
        nullptr,
    };
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    config.SecurityPolicy = &oversizedPolicy;
    store = SolidSyslogBlockStore_Create(&config);

    const char body[] = "HELLO WORLD";
    const size_t bodyLen = sizeof(body) - 1;
    CHECK_TRUE(SolidSyslogStore_Write(store, body, bodyLen));

    const size_t RECORD_HEADER = 4;
    const size_t SENT_FLAG = 1;
    LONGS_EQUAL(RECORD_HEADER + bodyLen + SENT_FLAG, FileFake_FileSize());
    MEMCMP_EQUAL(body, static_cast<const uint8_t*>(FileFake_FileContent()) + RECORD_HEADER, bodyLen);
}

/* ------------------------------------------------------------------
 * Error paths
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP_BASE(SolidSyslogBlockStoreErrors, BlockDeviceTestBase)
{
    struct SolidSyslogStore*   store   = nullptr;

    void setup() override
    {
        setupBlockDeviceFakes();
    }

    void teardown() override
    {
        SolidSyslogBlockStore_Destroy(store);
        teardownBlockDeviceFakes();
    }
};

// clang-format on

TEST(SolidSyslogBlockStoreErrors, OpenFailureStillReturnsNonNull)
{
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    FileFake_FailNextOpen(file);
    store = SolidSyslogBlockStore_Create(&config);
    CHECK_TRUE(store != nullptr);
}

TEST(SolidSyslogBlockStoreErrors, TransientOpenFailureRecoversOnNextWrite)
{
    /* BlockDevice opens lazily — a one-shot Open failure during Create heals
     * on the next operation that needs the file. */
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    FileFake_FailNextOpen(file);
    store = SolidSyslogBlockStore_Create(&config);
    CHECK_TRUE(SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN));
}

TEST(SolidSyslogBlockStoreErrors, WriteReturnsFalseOnWriteFailure)
{
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    store = SolidSyslogBlockStore_Create(&config);
    FileFake_FailNextWrite(file);
    CHECK_FALSE(SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN));
}

TEST(SolidSyslogBlockStoreErrors, ReadReturnsFalseOnReadFailure)
{
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    store = SolidSyslogBlockStore_Create(&config);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    FileFake_FailNextRead(file);

    char buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    CHECK_FALSE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    LONGS_EQUAL(0, bytesRead);
}

TEST(SolidSyslogBlockStoreErrors, MarkSentDoesNotAdvanceWhenWriteFails)
{
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    store = SolidSyslogBlockStore_Create(&config);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);

    char buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);

    FileFake_FailNextWrite(file);
    SolidSyslogStore_MarkSent(store);

    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
}

/* ------------------------------------------------------------------
 * Block rotation
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP_BASE(SolidSyslogBlockStoreRotation, BlockDeviceTestBase)
{
    static const size_t RECORD_OVERHEAD    = 5; /* 2 (magic) + 2 (length) + 1 (sent flag) */
    static const size_t ONE_MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + RECORD_OVERHEAD;

    struct SolidSyslogStore* store = nullptr;
    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};

    void setup() override
    {
        setupBlockDeviceFakes();
        memset(maxMsg, 'A', sizeof(maxMsg));
    }

    void teardown() override
    {
        SolidSyslogBlockStore_Destroy(store);
        teardownBlockDeviceFakes();
    }

    void CreateWithMaxBlockSize(size_t maxBlockSize, enum SolidSyslogDiscardPolicy policy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST,
                               size_t maxBlocks = 2,
                               SolidSyslogStoreFullCallback onStoreFull = nullptr, void* storeFullContext = nullptr)
    {
        EnsureDeviceBlockSize(maxBlockSize);
        struct SolidSyslogBlockStoreConfig config = DEFAULT_CONFIG;
        config.BlockDevice       = device;
        config.MaxBlocks          = maxBlocks;
        config.DiscardPolicy     = policy;
        config.OnStoreFull       = onStoreFull;
        config.StoreFullContext  = storeFullContext;
        store = SolidSyslogBlockStore_Create(&config);
    }

    void WriteMaxMsg()
    {
        SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg));
    }
};

// clang-format on

TEST(SolidSyslogBlockStoreRotation, WriteRotatesToNewBlockWhenFull)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg();
    WriteMaxMsg();
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store01.log"));
}

TEST(SolidSyslogBlockStoreRotation, WriteDoesNotRotateWhenBlockHasSpace)
{
    CreateWithMaxBlockSize(2 * ONE_MAX_MSG_RECORD);
    WriteMaxMsg();
    WriteMaxMsg();
    CHECK_FALSE(SolidSyslogFile_Exists(file, "/tmp/test_store01.log"));
}

TEST(SolidSyslogBlockStoreRotation, HasUnsentReturnsTrueAfterRotation)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg();
    WriteMaxMsg();
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreRotation, ReadReturnsFirstBlockAfterRotation)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg));

    WriteMaxMsg(); /* rotates to block 01 */

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);

    LONGS_EQUAL(sizeof(firstMsg), bytesRead);
    BYTES_EQUAL('B', buf[0]);
}

TEST(SolidSyslogBlockStoreRotation, MarkSentAdvancesReadToSecondBlock)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg));

    WriteMaxMsg(); /* rotates to block 01 */

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);

    LONGS_EQUAL(SOLIDSYSLOG_MAX_MESSAGE_SIZE, bytesRead);
    BYTES_EQUAL('A', buf[0]);
}

TEST(SolidSyslogBlockStoreRotation, FullDrainAcrossTwoBlocksHasUnsentFalse)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg();
    WriteMaxMsg();

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t bytesRead = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreRotation, DiscardOldestDeletesOldestBlockWhenAtMaxBlocks)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* block 00 */
    WriteMaxMsg(); /* block 01 — now at maxBlocks=2 */
    WriteMaxMsg(); /* block 02 — must discard 00 */

    CHECK_FALSE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store02.log"));
}

TEST(SolidSyslogBlockStoreRotation, DiscardOldestSurvivingDataIsReadable)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg)); /* block 00 */

    char secondMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(secondMsg, 'C', sizeof(secondMsg));
    SolidSyslogStore_Write(store, secondMsg, sizeof(secondMsg)); /* block 01 */

    WriteMaxMsg(); /* block 02 — discards 00 */

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);

    LONGS_EQUAL(SOLIDSYSLOG_MAX_MESSAGE_SIZE, bytesRead);
    BYTES_EQUAL('C', buf[0]);
}

TEST(SolidSyslogBlockStoreRotation, DiscardOldestDrainYieldsOnlySurvivingRecords)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg)); /* block 00 — will be discarded */

    char secondMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(secondMsg, 'C', sizeof(secondMsg));
    SolidSyslogStore_Write(store, secondMsg, sizeof(secondMsg)); /* block 01 — survives */

    WriteMaxMsg(); /* block 02 — triggers discard of block 00 */

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 0;

    /* First record should be from surviving block 01, not discarded block 00 */
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    BYTES_EQUAL('C', buf[0]);
    SolidSyslogStore_MarkSent(store);

    /* Second record from block 02 */
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    BYTES_EQUAL('A', buf[0]);
    SolidSyslogStore_MarkSent(store);

    /* No more records */
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreRotation, DiscardNewestReturnsFalseWhenAtMaxBlocks)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_DISCARD_POLICY_NEWEST);
    WriteMaxMsg(); /* block 00 */
    WriteMaxMsg(); /* block 01 — now at maxBlocks=2 */

    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
}

static int StoreFullCallbackCallCount;

static void StoreFullCallback(void* context)
{
    (void) context;
    StoreFullCallbackCallCount++;
}

TEST(SolidSyslogBlockStoreRotation, HaltInvokesCallbackWhenStoreFull)
{
    StoreFullCallbackCallCount = 0;
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_DISCARD_POLICY_HALT, 2, StoreFullCallback);

    WriteMaxMsg(); /* block 00 */
    WriteMaxMsg(); /* block 01 — now at maxBlocks=2 */

    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
    CALLED_FUNCTION(StoreFullCallback, ONCE);
}

TEST(SolidSyslogBlockStoreRotation, HaltWithNullCallbackDoesNotCrash)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_DISCARD_POLICY_HALT);

    WriteMaxMsg(); /* block 00 */
    WriteMaxMsg(); /* block 01 — now at maxBlocks=2 */

    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
}

TEST(SolidSyslogBlockStoreRotation, HaltSetsIsHaltedTrue)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_DISCARD_POLICY_HALT);

    WriteMaxMsg(); /* block 00 */
    WriteMaxMsg(); /* block 01 — now at maxBlocks=2 */

    CHECK_FALSE(SolidSyslogStore_IsHalted(store));
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* triggers halt */
    CHECK_TRUE(SolidSyslogStore_IsHalted(store));
}

TEST(SolidSyslogBlockStoreRotation, DiscardNewestDoesNotInvokeCallback)
{
    StoreFullCallbackCallCount = 0;
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_DISCARD_POLICY_NEWEST, 2, StoreFullCallback);

    WriteMaxMsg(); /* block 00 */
    WriteMaxMsg(); /* block 01 — now at maxBlocks=2 */

    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
    CALLED_FUNCTION(StoreFullCallback, NEVER);
}

static int CountStoreFullInvocationsCallCount;

static void CountStoreFullInvocations(void* context)
{
    (void) context;
    CountStoreFullInvocationsCallCount++;
}

TEST(SolidSyslogBlockStoreRotation, HaltOnStoreFullFiresOncePerRisingEdge)
{
    CountStoreFullInvocationsCallCount = 0;
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_DISCARD_POLICY_HALT, 2, CountStoreFullInvocations);

    WriteMaxMsg(); /* block 00 */
    WriteMaxMsg(); /* block 01 — now at maxBlocks=2 */

    /* Three consecutive failed Writes — callback must fire on the first only. */
    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));

    CALLED_FUNCTION(CountStoreFullInvocations, ONCE);
}

static void* storeFullCallbackContext;

static void StoreFullCallbackCapturingContext(void* context)
{
    storeFullCallbackContext = context;
}

/* Given the integrator wires storeFullContext at config time,
 * When onStoreFull fires,
 * Then the callback receives the configured context pointer unchanged. */
TEST(SolidSyslogBlockStoreRotation, OnStoreFullReceivesConfiguredContext)
{
    int sentinel = 0;
    storeFullCallbackContext = nullptr;

    CreateWithMaxBlockSize(
        ONE_MAX_MSG_RECORD,
        SOLIDSYSLOG_DISCARD_POLICY_HALT,
        2,
        StoreFullCallbackCapturingContext,
        &sentinel
    );

    WriteMaxMsg(); /* block 00 */
    WriteMaxMsg(); /* block 01 — at maxBlocks */

    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* triggers halt callback */

    POINTERS_EQUAL(&sentinel, storeFullCallbackContext);
}

TEST(SolidSyslogBlockStoreRotation, ResumeHasUnsentWhenMultipleBlocksExist)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* block 00 */
    WriteMaxMsg(); /* block 01 */
    SolidSyslogBlockStore_Destroy(store);

    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreRotation, ResumeDrainsAcrossBlocksInOrder)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg)); /* block 00 */

    WriteMaxMsg(); /* block 01 — 'A' */
    SolidSyslogBlockStore_Destroy(store);

    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    BYTES_EQUAL('B', buf[0]);
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    BYTES_EQUAL('A', buf[0]);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreRotation, ResumeContinuesWritingToCorrectBlock)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* block 00 */
    SolidSyslogBlockStore_Destroy(store);

    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* should rotate to block 01, not overwrite 00 */

    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store01.log"));
}

TEST(SolidSyslogBlockStoreRotation, ResumeWithMultipleBlocksCanWriteNewMessage)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* block 00 */
    WriteMaxMsg(); /* block 01 */
    SolidSyslogBlockStore_Destroy(store);

    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);

    char newMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(newMsg, 'N', sizeof(newMsg));
    CHECK_TRUE(SolidSyslogStore_Write(store, newMsg, sizeof(newMsg)));

    /* Should have rotated to block 02 — block 01 was full */
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store02.log"));
}

TEST(SolidSyslogBlockStoreRotation, ResumeWriteAppendsToPartiallyFilledWriteBlock)
{
    static const size_t TWO_MAX_MSG_RECORDS = 2 * ONE_MAX_MSG_RECORD;

    CreateWithMaxBlockSize(TWO_MAX_MSG_RECORDS);
    WriteMaxMsg(); /* block 00, record 1 */
    WriteMaxMsg(); /* block 00, record 2 — block 00 full */
    WriteMaxMsg(); /* block 01, record 1 — block 01 partially filled */
    SolidSyslogBlockStore_Destroy(store);

    CreateWithMaxBlockSize(TWO_MAX_MSG_RECORDS);

    /* Write should append to block 01, not rotate */
    CHECK_TRUE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
    CHECK_FALSE(SolidSyslogFile_Exists(file, "/tmp/test_store02.log"));
}

/* Regression: when the read block is closed/full and the write block is
 * partially filled, the resume scan must bound by the read block's actual
 * size, not by WritePosition (which is the size of the write block). Prior
 * to the fix the scan stopped at WritePosition and a sent record in the
 * read block could be re-emitted on resume. */
TEST(SolidSyslogBlockStoreRotation, ResumeFindsUnsentInClosedReadBlockWhenWriteBlockPartial)
{
    /* Three max-msg records per block so the read block holds multiple sent
     * records past WritePosition (== size of the partially-filled write block). */
    static const size_t THREE_MAX_MSG_RECORDS = 3 * ONE_MAX_MSG_RECORD;

    CreateWithMaxBlockSize(THREE_MAX_MSG_RECORDS, SOLIDSYSLOG_DISCARD_POLICY_OLDEST, /* maxBlocks */ 3);

    char msg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    for (char i = 0; i < 4; i++)
    {
        memset(msg, 'A' + i, sizeof(msg));
        SolidSyslogStore_Write(store, msg, sizeof(msg));
    }
    /* records 0..2 fill block 0; record 3 lands in block 1 (partially full) */

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 0;
    for (int i = 0; i < 2; i++)
    {
        SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
        SolidSyslogStore_MarkSent(store);
    }
    /* records 0, 1 in block 0 marked sent; record 2 still unsent */

    SolidSyslogBlockStore_Destroy(store);
    CreateWithMaxBlockSize(THREE_MAX_MSG_RECORDS, SOLIDSYSLOG_DISCARD_POLICY_OLDEST, 3);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    BYTES_EQUAL('A' + 2, buf[0]); /* record 2, not a re-emit of record 1 */
}

TEST(SolidSyslogBlockStoreRotation, SequenceWrapsFrom99To00)
{
    /* Pre-seed block 99 on disk so the scan finds it as the write block */
    SolidSyslogFile_Open(file, "/tmp/test_store99.log");
    SolidSyslogFile_Close(file);

    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* fills block 99 */
    WriteMaxMsg(); /* should wrap to block 00 */

    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));
}

TEST(SolidSyslogBlockStoreRotation, WriteAfterDrainRotatesToNextBlock)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg();

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));

    /* Drained block still occupies space — next write rotates */
    WriteMaxMsg();
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store01.log"));
}

TEST(SolidSyslogBlockStoreRotation, MixedMessageSizesDrainCorrectlyAcrossBlocks)
{
    static const size_t SHORT_LEN = 7;

    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);

    char shortMsg[SHORT_LEN];
    memset(shortMsg, 'S', SHORT_LEN);
    SolidSyslogStore_Write(store, shortMsg, SHORT_LEN); /* block 00 — small record */

    WriteMaxMsg(); /* block 01 — max record */

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    LONGS_EQUAL(SHORT_LEN, bytesRead);
    BYTES_EQUAL('S', buf[0]);
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    LONGS_EQUAL(SOLIDSYSLOG_MAX_MESSAGE_SIZE, bytesRead);
    BYTES_EQUAL('A', buf[0]);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreRotation, ContinuousDiscardWithoutReadingSurvivorsCorrect)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);

    /* Write 5 messages across 5 blocks — maxBlocks=2 means 3 are discarded */
    char msgs[5][SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index) -- loop index is bounded by literal 5
    for (int i = 0; i < 5; i++)
    {
        memset(msgs[i], 'A' + i, sizeof(msgs[i]));
        SolidSyslogStore_Write(store, msgs[i], sizeof(msgs[i]));
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

    /* Only blocks 03 and 04 should survive */
    CHECK_FALSE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));
    CHECK_FALSE(SolidSyslogFile_Exists(file, "/tmp/test_store01.log"));
    CHECK_FALSE(SolidSyslogFile_Exists(file, "/tmp/test_store02.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store03.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store04.log"));

    /* Drain — should get msg3 ('D') then msg4 ('E') */
    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    BYTES_EQUAL('D', buf[0]);
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    BYTES_EQUAL('E', buf[0]);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreRotation, MaxBlocksAtUpperLimit)
{
    enum
    {
        MAX_BLOCKS = 99
    };

    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_DISCARD_POLICY_OLDEST, MAX_BLOCKS);

    /* Fill all 99 blocks */
    for (int i = 0; i < MAX_BLOCKS; i++)
    {
        WriteMaxMsg();
    }

    /* All 99 blocks should exist (00–98) */
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store98.log"));
    CHECK_FALSE(SolidSyslogFile_Exists(file, "/tmp/test_store99.log"));

    /* One more write — should discard block 00 and create block 99 */
    WriteMaxMsg();

    CHECK_FALSE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store99.log"));
}

TEST(SolidSyslogBlockStoreRotation, MultipleRecordsPerBlockDrainAcrossRotation)
{
    static const size_t TWO_MAX_MSG_RECORDS = 2 * ONE_MAX_MSG_RECORD;

    CreateWithMaxBlockSize(TWO_MAX_MSG_RECORDS);

    char msg0[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(msg0, 'X', sizeof(msg0));
    SolidSyslogStore_Write(store, msg0, sizeof(msg0)); /* block 00, record 1 */

    char msg1[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(msg1, 'Y', sizeof(msg1));
    SolidSyslogStore_Write(store, msg1, sizeof(msg1)); /* block 00, record 2 */

    WriteMaxMsg(); /* block 01, record 1 — 'A' */

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    BYTES_EQUAL('X', buf[0]);
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    BYTES_EQUAL('Y', buf[0]);
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    BYTES_EQUAL('A', buf[0]);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreRotation, MarkSentDisposesOlderBlockWhenDrained)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* block 00 */
    WriteMaxMsg(); /* rotates to block 01 */

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));
}

TEST(SolidSyslogBlockStoreRotation, MarkSentDoesNotDisposeActiveWriteBlock)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* block 00 — also the active write block */

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));
}

TEST(SolidSyslogBlockStoreRotation, RotationDisposesPriorBlockWhenAlreadyDrained)
{
    /* Interleaved drain pattern: MarkSent fires for the only record in block 00
     * while it is still the active write block, so dispose-on-empty cannot fire
     * yet. The trigger must re-evaluate after the next Write rotates writeSequence
     * to 01 — otherwise the just-filled-and-drained block lingers until capacity
     * pressure forces discard. This pattern is what the threaded service thread
     * does in practice. */
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);

    WriteMaxMsg();
    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));

    WriteMaxMsg(); /* triggers rotation; block 00 becomes non-active */

    CHECK_FALSE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));
}

TEST(SolidSyslogBlockStoreRotation, WriteReturnsFalseWhenRotationAcquireFails)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* fills block 00 */

    FileFake_FailNextOpen(file); /* next Acquire on rotation will fail */
    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
    CHECK_FALSE(SolidSyslogFile_Exists(file, "/tmp/test_store01.log"));
}

TEST(SolidSyslogBlockStoreRotation, RotationRetriesAfterTransientAcquireFailure)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* block 00 */

    FileFake_FailNextOpen(file);
    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg))); /* fails — Acquire on block 01 rejected */

    CHECK_TRUE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg))); /* retry succeeds */
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store01.log"));
}

TEST(SolidSyslogBlockStoreRotation, DiscardRetriesAfterTransientDisposeFailure)
{
    /* Without this guarantee the oldest pointer would advance past a still-on-disk
     * block, leaving it orphaned forever. Force a Dispose failure on the discard
     * during block-02 rotation, then trigger another rotation: the next discard
     * cycle must re-attempt block 00 — not skip past it to block 01. Both Writes
     * succeed (rotation acquires the new block; only the discard silently fails). */
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* block 00 */
    WriteMaxMsg(); /* block 01 */

    FileFake_FailNextDelete(file);
    CHECK_TRUE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg))
    ); /* block 02 — discard of 00 fails; oldest must stay at 0 */
    CHECK_TRUE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg))
    ); /* block 03 — next discard cycle re-attempts and removes 00 */

    CHECK_FALSE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));
}

/* ------------------------------------------------------------------
 * Integrity (SecurityPolicy integration)
 * ----------------------------------------------------------------*/

enum
{
    CONTENT_REGION_MAX = 2 + 2 + SOLIDSYSLOG_MAX_MESSAGE_SIZE /* magic + length + body */
};

static int SpySealRecordCallCount;
static uint8_t sealContentData[CONTENT_REGION_MAX];
static uint16_t sealContentLength;
static uint16_t sealHeaderLength;

static bool SpySealRecord(struct SolidSyslogSecurityPolicy* self, const struct SolidSyslogSecurityRecord* record)
{
    (void) self;
    SpySealRecordCallCount++;
    sealContentLength = record->ContentLength;
    sealHeaderLength = record->HeaderLength;
    memcpy(sealContentData, record->Content, record->ContentLength);
    return true;
}

static int SpyOpenRecordCallCount;
static uint8_t openContentData[CONTENT_REGION_MAX];
static uint16_t openContentLength;
static uint16_t openHeaderLength;

static bool SpyOpenRecord(struct SolidSyslogSecurityPolicy* self, const struct SolidSyslogSecurityRecord* record)
{
    (void) self;
    SpyOpenRecordCallCount++;
    openContentLength = record->ContentLength;
    openHeaderLength = record->HeaderLength;
    memcpy(openContentData, record->Content, record->ContentLength);
    return true;
}

static struct SolidSyslogSecurityPolicy spyPolicy = {
    0,
    SpySealRecord,
    SpyOpenRecord,
};

// clang-format off
TEST_GROUP_BASE(SolidSyslogBlockStoreIntegrity, BlockDeviceTestBase)
{
    struct SolidSyslogStore* store = nullptr;

    void setup() override
    {
        setupBlockDeviceFakes();
        SpySealRecordCallCount  = 0;
        sealContentLength  = 0;
        sealHeaderLength  = 0;
        memset(sealContentData, 0, sizeof(sealContentData));
        SpyOpenRecordCallCount   = 0;
        openContentLength   = 0;
        openHeaderLength   = 0;
        memset(openContentData, 0, sizeof(openContentData));

        struct SolidSyslogBlockStoreConfig config = DEFAULT_CONFIG;
        config.BlockDevice    = device;
        config.SecurityPolicy = &spyPolicy;
        store = SolidSyslogBlockStore_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogBlockStore_Destroy(store);
        teardownBlockDeviceFakes();
    }
};

// clang-format on

TEST(SolidSyslogBlockStoreIntegrity, WriteCallsSealRecord)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    CALLED_FUNCTION(SpySealRecord, ONCE);
}

TEST(SolidSyslogBlockStoreIntegrity, SealRecordReceivesContentRegionAndHeaderSplit)
{
    enum
    {
        MAGIC_SIZE = 2,
        LENGTH_SIZE = 2,
        HEADER_SIZE = MAGIC_SIZE + LENGTH_SIZE,
        REGION_SIZE = HEADER_SIZE + TEST_DATA_LEN
    };

    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    LONGS_EQUAL(REGION_SIZE, sealContentLength);

    /* magic|length is the cleartext header; message is the body */
    LONGS_EQUAL(HEADER_SIZE, sealHeaderLength);

    /* verify magic bytes */
    BYTES_EQUAL(0xA5, sealContentData[0]);
    BYTES_EQUAL(0x5A, sealContentData[1]);

    /* verify body is included */
    MEMCMP_EQUAL(TEST_DATA, sealContentData + HEADER_SIZE, TEST_DATA_LEN);
}

TEST(SolidSyslogBlockStoreIntegrity, ReadCallsOpenRecord)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    CALLED_FUNCTION(SpyOpenRecord, ONCE);
}

TEST(SolidSyslogBlockStoreIntegrity, OpenRecordReceivesContentRegionAndHeaderSplit)
{
    enum
    {
        MAGIC_SIZE = 2,
        LENGTH_SIZE = 2,
        HEADER_SIZE = MAGIC_SIZE + LENGTH_SIZE,
        REGION_SIZE = HEADER_SIZE + TEST_DATA_LEN
    };

    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    LONGS_EQUAL(REGION_SIZE, openContentLength);

    /* magic|length is the cleartext header; message is the body */
    LONGS_EQUAL(HEADER_SIZE, openHeaderLength);

    /* verify magic bytes */
    BYTES_EQUAL(0xA5, openContentData[0]);
    BYTES_EQUAL(0x5A, openContentData[1]);

    /* verify body is included */
    MEMCMP_EQUAL(TEST_DATA, openContentData + HEADER_SIZE, TEST_DATA_LEN);
}

/* ------------------------------------------------------------------
 * Corruption detection
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP_BASE(SolidSyslogBlockStoreCorruption, BlockDeviceTestBase)
{
    struct SolidSyslogStore* store = nullptr;

    void setup() override
    {
        setupBlockDeviceFakes();
    }

    void teardown() override
    {
        SolidSyslogBlockStore_Destroy(store);
        teardownBlockDeviceFakes();
    }

    void WriteRawBytes(const char* path, const void* data, size_t size) const
    {
        SolidSyslogFile_Open(file, path);
        SolidSyslogFile_Write(file, data, size);
        SolidSyslogFile_Close(file);
    }

    void CreateStore()
    {
        struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
        store = SolidSyslogBlockStore_Create(&config);
    }
};

// clang-format on

TEST(SolidSyslogBlockStoreCorruption, TruncatedMagicHasNoUnsent)
{
    uint8_t oneByte = 0xA5;
    WriteRawBytes("/tmp/test_store00.log", &oneByte, 1);
    CreateStore();
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreCorruption, BadMagicHasNoUnsent)
{
    uint8_t badMagic[] = {0x00, 0x00, 0x05, 0x00, 'h', 'e', 'l', 'l', 'o', 0xFF};
    WriteRawBytes("/tmp/test_store00.log", badMagic, sizeof(badMagic));
    CreateStore();
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreCorruption, TruncatedLengthHasNoUnsent)
{
    uint8_t truncatedHeader[] = {0xA5, 0x5A, 0x05};
    WriteRawBytes("/tmp/test_store00.log", truncatedHeader, sizeof(truncatedHeader));
    CreateStore();
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreCorruption, TruncatedBodyHasNoUnsent)
{
    /* valid magic + length=5, but only 2 bytes of body, no integrity or sent flag */
    uint8_t truncatedBody[] = {0xA5, 0x5A, 0x05, 0x00, 'h', 'e'};
    WriteRawBytes("/tmp/test_store00.log", truncatedBody, sizeof(truncatedBody));
    CreateStore();
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogBlockStoreCorruption, ValidRecordBeforeCorruptionIsReadable)
{
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    config.SecurityPolicy = SolidSyslogCrc16Policy_Create();
    store = SolidSyslogBlockStore_Create(&config);
    SolidSyslogStore_Write(store, "first", 5);
    SolidSyslogStore_Write(store, "second", 6);
    SolidSyslogBlockStore_Destroy(store);

    /* Corrupt the second record's body */
    enum
    {
        MAGIC = 2,
        LENGTH = 2,
        FIRST_BODY = 5,
        CRC = 2,
        SENT = 1,
        SECOND_RECORD_BODY_OFFSET = MAGIC + LENGTH + FIRST_BODY + CRC + SENT + MAGIC + LENGTH
    };

    SolidSyslogFile_Open(file, "/tmp/test_store00.log");
    uint8_t corrupt = 0xFF;
    SolidSyslogFile_SeekTo(file, SECOND_RECORD_BODY_OFFSET);
    SolidSyslogFile_Write(file, &corrupt, 1);
    SolidSyslogFile_Close(file);

    /* Re-open: first record is valid, second is corrupt */
    store = SolidSyslogBlockStore_Create(&config);
    char buf[TEST_BUF_SIZE] = {};
    size_t bytesRead = 0;

    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    LONGS_EQUAL(5, bytesRead);
    MEMCMP_EQUAL("first", buf, 5);
}

TEST(SolidSyslogBlockStoreCorruption, IntegrityFailureReadReturnsFalse)
{
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    config.SecurityPolicy = SolidSyslogCrc16Policy_Create();
    store = SolidSyslogBlockStore_Create(&config);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    SolidSyslogBlockStore_Destroy(store);

    /* Corrupt one byte of the body in the stored record */
    SolidSyslogFile_Open(file, "/tmp/test_store00.log");
    uint8_t corrupt = 0xFF;
    SolidSyslogFile_SeekTo(file, 4); /* offset past magic(2) + length(2) */
    SolidSyslogFile_Write(file, &corrupt, 1);
    SolidSyslogFile_Close(file);

    store = SolidSyslogBlockStore_Create(&config);
    char buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    CHECK_FALSE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
}

TEST(SolidSyslogBlockStoreCorruption, InvalidLengthReadReturnsFalse)
{
    /* Write many records to make the file large enough that a bogus length
     * doesn't hit EOF — the length check must reject it explicitly */
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    store = SolidSyslogBlockStore_Create(&config);

    char largeMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(largeMsg, 'X', sizeof(largeMsg));
    SolidSyslogStore_Write(store, largeMsg, sizeof(largeMsg));
    SolidSyslogStore_Write(store, largeMsg, sizeof(largeMsg));
    SolidSyslogBlockStore_Destroy(store);

    /* Overwrite the length field of the first record (bytes 2-3) */
    SolidSyslogFile_Open(file, "/tmp/test_store00.log");
    uint16_t badLength = SOLIDSYSLOG_MAX_MESSAGE_SIZE + 1;
    SolidSyslogFile_SeekTo(file, 2);
    SolidSyslogFile_Write(file, &badLength, 2);
    SolidSyslogFile_Close(file);

    store = SolidSyslogBlockStore_Create(&config);
    char buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    CHECK_FALSE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
}

/* ------------------------------------------------------------------
 * Corruption recovery
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP_BASE(SolidSyslogBlockStoreCorruptionRecovery, BlockDeviceTestBase)
{
    static const size_t RECORD_OVERHEAD    = 7; /* 2 (magic) + 2 (length) + 2 (crc) + 1 (sent) */
    static const size_t ONE_MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + RECORD_OVERHEAD;

    struct SolidSyslogStore* store = nullptr;
    struct SolidSyslogSecurityPolicy* policy = nullptr;
    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};

    void setup() override
    {
        setupBlockDeviceFakes();
        policy = SolidSyslogCrc16Policy_Create();
        memset(maxMsg, 'A', sizeof(maxMsg));
    }

    void teardown() override
    {
        SolidSyslogBlockStore_Destroy(store);
        teardownBlockDeviceFakes();
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- maxBlockSize and maxBlocks have distinct semantics
    void CreateWithMaxBlockSize(size_t maxBlockSize, size_t maxBlocks = 2)
    {
        EnsureDeviceBlockSize(maxBlockSize);
        struct SolidSyslogBlockStoreConfig config = DEFAULT_CONFIG;
        config.BlockDevice     = device;
        config.MaxBlocks        = maxBlocks;
        config.SecurityPolicy  = policy;
        store = SolidSyslogBlockStore_Create(&config);
    }

    void WriteMaxMsg()
    {
        SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg));
    }

    void CorruptFirstRecordBody(const char* path) const
    {
        SolidSyslogFile_Open(file, path);
        uint8_t corrupt = 0xFF;
        SolidSyslogFile_SeekTo(file, 4);
        SolidSyslogFile_Write(file, &corrupt, 1);
        SolidSyslogFile_Close(file);
    }
};

// clang-format on

TEST(SolidSyslogBlockStoreCorruptionRecovery, ReadSkipsCorruptOlderBlockToNextBlock)
{
    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg)); /* block 00 */

    WriteMaxMsg(); /* block 01 */
    SolidSyslogBlockStore_Destroy(store);

    CorruptFirstRecordBody("/tmp/test_store00.log");

    CreateWithMaxBlockSize(ONE_MAX_MSG_RECORD);

    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 0;
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    LONGS_EQUAL(SOLIDSYSLOG_MAX_MESSAGE_SIZE, bytesRead);
    BYTES_EQUAL('A', buf[0]);
}

TEST(SolidSyslogBlockStoreCorruptionRecovery, CorruptWriteBlockRotatesOnNextWrite)
{
    /* Use a block size that fits two records — the first write leaves space,
     * so rotation on the second write proves corruption forced it */
    static const size_t TWO_MAX_MSG_RECORDS = 2 * ONE_MAX_MSG_RECORD;

    CreateWithMaxBlockSize(TWO_MAX_MSG_RECORDS);
    WriteMaxMsg(); /* block 00 — partially filled */
    SolidSyslogBlockStore_Destroy(store);

    CorruptFirstRecordBody("/tmp/test_store00.log");

    CreateWithMaxBlockSize(TWO_MAX_MSG_RECORDS);

    /* Block 00 has space but is corrupt — write should rotate to block 01 */
    char newMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(newMsg, 'N', sizeof(newMsg));
    CHECK_TRUE(SolidSyslogStore_Write(store, newMsg, sizeof(newMsg)));
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store01.log"));
}

/* ------------------------------------------------------------------
 * Capacity getters
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP_BASE(SolidSyslogBlockStoreCapacity, BlockDeviceTestBase)
{
    static const size_t ONE_MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + TEST_RECORD_OVERHEAD;

    struct SolidSyslogStore* store = nullptr;
    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};

    void setup() override
    {
        setupBlockDeviceFakes();
        memset(maxMsg, 'A', sizeof(maxMsg));
    }

    void teardown() override
    {
        if (store != nullptr) { SolidSyslogBlockStore_Destroy(store); }
        teardownBlockDeviceFakes();
    }

    void CreateDefault()
    {
        struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
        store = SolidSyslogBlockStore_Create(&config);
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- maxBlockSize is a byte size, maxBlocks is a count; distinct semantics
    void CreateWithCapacity(size_t maxBlockSize, size_t maxBlocks,
                            enum SolidSyslogDiscardPolicy policy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST)
    {
        EnsureDeviceBlockSize(maxBlockSize);
        struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
        config.MaxBlocks      = maxBlocks;
        config.DiscardPolicy = policy;
        store                = SolidSyslogBlockStore_Create(&config);
    }

    void WriteMaxMsg()
    {
        SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg));
    }
};

// clang-format on

/* Given maxBlocks × maxBlockSize configured,
 * When GetTotalBytes is queried,
 * Then it returns the product. */
TEST(SolidSyslogBlockStoreCapacity, GetTotalBytesReturnsMaxBlocksTimesMaxBlockSize)
{
    CreateDefault();
    LONGS_EQUAL(TEST_MAX_BLOCKS * TEST_MAX_BLOCK_SIZE, SolidSyslogStore_GetTotalBytes(store));
}

TEST(SolidSyslogBlockStoreCapacity, GetTotalBytesScalesWithConfig)
{
    CreateWithCapacity(10000, 3);
    LONGS_EQUAL(3 * 10000, SolidSyslogStore_GetTotalBytes(store));
}

TEST(SolidSyslogBlockStoreCapacity, TotalBytesDerivesFromDeviceBlockSize)
{
    EnsureDeviceBlockSize(8192);
    CreateDefault();
    LONGS_EQUAL(TEST_MAX_BLOCKS * 8192, SolidSyslogStore_GetTotalBytes(store));
}

/* Given an empty store,
 * When GetUsedBytes is queried,
 * Then it returns 0. */
TEST(SolidSyslogBlockStoreCapacity, GetUsedBytesIsZeroOnEmptyStore)
{
    CreateDefault();
    LONGS_EQUAL(0, SolidSyslogStore_GetUsedBytes(store));
}

/* Given an empty store,
 * When records totalling X bytes are written,
 * Then GetUsedBytes returns X (including record overhead). */
TEST(SolidSyslogBlockStoreCapacity, GetUsedBytesTracksOneWrite)
{
    CreateDefault();
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    LONGS_EQUAL(TEST_DATA_LEN + TEST_RECORD_OVERHEAD, SolidSyslogStore_GetUsedBytes(store));
}

TEST(SolidSyslogBlockStoreCapacity, GetUsedBytesCountsClosedBlocksAtFullSize)
{
    /* Tight maxBlockSize so a single max-msg record fills a block; second write
     * rotates. Closed block contributes maxBlockSize regardless of slack. */
    CreateWithCapacity(ONE_MAX_MSG_RECORD, 3);
    WriteMaxMsg(); /* block 0 fills */
    WriteMaxMsg(); /* rotates to block 1 */

    LONGS_EQUAL(ONE_MAX_MSG_RECORD + ONE_MAX_MSG_RECORD, SolidSyslogStore_GetUsedBytes(store));
}

/* Given a full store with SOLIDSYSLOG_DISCARD_POLICY_OLDEST,
 * When the oldest block is discarded,
 * Then GetUsedBytes drops by one block size. */
TEST(SolidSyslogBlockStoreCapacity, GetUsedBytesDropsOnDiscardOldest)
{
    CreateWithCapacity(ONE_MAX_MSG_RECORD, 2);
    WriteMaxMsg(); /* block 0 full */
    WriteMaxMsg(); /* block 1 full, at maxBlocks */

    LONGS_EQUAL(2 * ONE_MAX_MSG_RECORD, SolidSyslogStore_GetUsedBytes(store));

    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN); /* rotates to block 2, discards block 0 */

    /* 1 closed block (block 1) + active block holds one small record. */
    LONGS_EQUAL(ONE_MAX_MSG_RECORD + TEST_DATA_LEN + TEST_RECORD_OVERHEAD, SolidSyslogStore_GetUsedBytes(store));
}

/* Given a store at capacity with SOLIDSYSLOG_DISCARD_POLICY_HALT,
 * When a Write fails for size,
 * Then GetUsedBytes returns total even when the active block has slack. */
TEST(SolidSyslogBlockStoreCapacity, GetUsedBytesIsStickyAtTotalAfterSizeFailure)
{
    /* maxBlockSize larger than one max-msg record so the active block has slack. */
    static const size_t SLACK = 100;
    CreateWithCapacity(ONE_MAX_MSG_RECORD + SLACK, 2, SOLIDSYSLOG_DISCARD_POLICY_HALT);
    WriteMaxMsg(); /* block 0: SLACK bytes slack */
    WriteMaxMsg(); /* block 1: SLACK bytes slack, at maxBlocks */

    /* The next write needs to rotate but can't (HALT, at maxBlocks) — fails for size. */
    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));

    /* Sticky: GetUsedBytes returns total even though the active blocks have slack. */
    LONGS_EQUAL(SolidSyslogStore_GetTotalBytes(store), SolidSyslogStore_GetUsedBytes(store));
}

/* ------------------------------------------------------------------
 * Capacity threshold alert (S05.09)
 * ----------------------------------------------------------------*/

static int CountThresholdCrossingsCallCount;
static size_t thresholdReturnValue;

static size_t ReturnsConfiguredThreshold(void* context)
{
    (void) context;
    return thresholdReturnValue;
}

static void CountThresholdCrossings(void* context)
{
    (void) context;
    CountThresholdCrossingsCallCount++;
}

// clang-format off
TEST_GROUP_BASE(SolidSyslogBlockStoreCapacityThreshold, BlockDeviceTestBase)
{
    struct SolidSyslogStore* store = nullptr;

    void setup() override
    {
        setupBlockDeviceFakes();
        CountThresholdCrossingsCallCount = 0;
        thresholdReturnValue   = 0;
    }

    void teardown() override
    {
        if (store != nullptr) { SolidSyslogBlockStore_Destroy(store); }
        teardownBlockDeviceFakes();
    }

    void CreateWithThreshold(size_t threshold)
    {
        struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
        config.GetCapacityThreshold              = ReturnsConfiguredThreshold;
        config.OnThresholdCrossed                = CountThresholdCrossings;
        thresholdReturnValue                     = threshold;
        store                                    = SolidSyslogBlockStore_Create(&config);
    }
};

// clang-format on

/* Given a threshold below the size of a single record's overhead,
 * When a write makes used-bytes cross the threshold,
 * Then onThresholdCrossed fires. */
TEST(SolidSyslogBlockStoreCapacityThreshold, FiresOnRisingEdgeCrossing)
{
    CreateWithThreshold(TEST_DATA_LEN);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    CALLED_FUNCTION(CountThresholdCrossings, ONCE);
}

/* Given usage already above threshold,
 * When subsequent writes keep usage above threshold,
 * Then onThresholdCrossed fires only on the rising edge. */
TEST(SolidSyslogBlockStoreCapacityThreshold, FiresOnceWhileUsageStaysAbove)
{
    CreateWithThreshold(TEST_DATA_LEN);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN); /* crosses */
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN); /* still above */
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN); /* still above */
    CALLED_FUNCTION(CountThresholdCrossings, ONCE);
}

/* Given DISCARD_OLDEST and a threshold sitting in the last block,
 * When writes cross the threshold, a discard drops usage below it, then writes cross again,
 * Then onThresholdCrossed fires twice. */
TEST(SolidSyslogBlockStoreCapacityThreshold, ReArmsAfterFallingEdgeOnDiscardOldest)
{
    static const size_t MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + TEST_RECORD_OVERHEAD;
    static const size_t TWO_RECORDS = 2 * MAX_MSG_RECORD;

    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(maxMsg, 'A', sizeof(maxMsg));

    EnsureDeviceBlockSize(TWO_RECORDS);
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    config.MaxBlocks = 2;
    config.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    config.GetCapacityThreshold = ReturnsConfiguredThreshold;
    config.OnThresholdCrossed = CountThresholdCrossings;
    /* Threshold sits between 3 and 4 records: 4-records crosses, 3-records is below. */
    thresholdReturnValue = (3 * MAX_MSG_RECORD) + 1;
    store = SolidSyslogBlockStore_Create(&config);

    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* block 0: 1 record */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* block 0: 2 records (full) */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* rotate; block 1: 1 record (3 total) */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* block 1: 2 records (4 total) — fires */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* rotate+discard block 0 → 3 records (below) */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* block 2: 2 records (4 total) — fires again */

    CALLED_FUNCTION(CountThresholdCrossings, TWICE);
}

/* Given getCapacityThreshold returns 0,
 * When usage rises arbitrarily,
 * Then onThresholdCrossed never fires. */
TEST(SolidSyslogBlockStoreCapacityThreshold, DoesNotFireWhenThresholdIsZero)
{
    CreateWithThreshold(0);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    CALLED_FUNCTION(CountThresholdCrossings, NEVER);
}

/* Given getCapacityThreshold is NULL but onThresholdCrossed is configured,
 * When usage rises arbitrarily,
 * Then onThresholdCrossed never fires (and the library does not deref a NULL function). */
TEST(SolidSyslogBlockStoreCapacityThreshold, DoesNotFireWhenThresholdFunctionIsNull)
{
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    config.GetCapacityThreshold = nullptr;
    config.OnThresholdCrossed = CountThresholdCrossings;
    store = SolidSyslogBlockStore_Create(&config);

    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);

    CALLED_FUNCTION(CountThresholdCrossings, NEVER);
}

static void* capturedThresholdFunctionContext;
static void* capturedThresholdCallbackContext;

static size_t CaptureThresholdFunctionContext(void* context)
{
    capturedThresholdFunctionContext = context;
    return thresholdReturnValue;
}

static void CaptureThresholdCallbackContext(void* context)
{
    capturedThresholdCallbackContext = context;
}

/* Given thresholdContext wired at config time,
 * When getCapacityThreshold and onThresholdCrossed are invoked,
 * Then both receive the configured context. */
TEST(SolidSyslogBlockStoreCapacityThreshold, ContextIsPassedToBothCallbacks)
{
    int sentinel = 0;
    capturedThresholdFunctionContext = nullptr;
    capturedThresholdCallbackContext = nullptr;
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    config.GetCapacityThreshold = CaptureThresholdFunctionContext;
    config.OnThresholdCrossed = CaptureThresholdCallbackContext;
    config.ThresholdContext = &sentinel;
    thresholdReturnValue = TEST_DATA_LEN;
    store = SolidSyslogBlockStore_Create(&config);

    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);

    POINTERS_EQUAL(&sentinel, capturedThresholdFunctionContext);
    POINTERS_EQUAL(&sentinel, capturedThresholdCallbackContext);
}

static int nextFireOrder;
static int thresholdFireOrder;
static int storeFullFireOrder;

static void RecordThresholdFireOrder(void* context)
{
    (void) context;
    thresholdFireOrder = ++nextFireOrder;
}

static void RecordStoreFullFireOrder(void* context)
{
    (void) context;
    storeFullFireOrder = ++nextFireOrder;
}

/* Given threshold = 100% (total bytes) and SOLIDSYSLOG_DISCARD_POLICY_HALT,
 * When a Write fails for size and engages the sticky 100% bit,
 * Then onThresholdCrossed fires before onStoreFull on that same Write. */
TEST(SolidSyslogBlockStoreCapacityThreshold, AtFullCapacityWithHaltThresholdFiresBeforeStoreFull)
{
    static const size_t MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + TEST_RECORD_OVERHEAD;
    static const size_t SLACK = 100;

    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(maxMsg, 'A', sizeof(maxMsg));

    nextFireOrder = 0;
    thresholdFireOrder = 0;
    storeFullFireOrder = 0;

    EnsureDeviceBlockSize(MAX_MSG_RECORD + SLACK);
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    config.MaxBlocks = 2;
    config.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_HALT;
    config.OnStoreFull = RecordStoreFullFireOrder;
    config.GetCapacityThreshold = ReturnsConfiguredThreshold;
    config.OnThresholdCrossed = RecordThresholdFireOrder;
    /* Threshold = total: only the sticky-100% engagement on a failed Write reaches it. */
    thresholdReturnValue = 2 * (MAX_MSG_RECORD + SLACK);
    store = SolidSyslogBlockStore_Create(&config);

    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* block 0 partially full */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* rotate; block 1 partially full */
    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg))); /* HALT: fails, sticky engages */

    CHECK_TRUE(thresholdFireOrder > 0);
    CHECK_TRUE(storeFullFireOrder > 0);
    CHECK_TRUE(thresholdFireOrder < storeFullFireOrder);
}

/* Given a failed Write has already engaged the sticky 100% bit,
 * When subsequent Writes also fail for size,
 * Then onThresholdCrossed does not fire again. */
TEST(SolidSyslogBlockStoreCapacityThreshold, StickyHundredPercentDoesNotRefireThreshold)
{
    static const size_t MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + TEST_RECORD_OVERHEAD;
    static const size_t SLACK = 100;

    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(maxMsg, 'A', sizeof(maxMsg));

    EnsureDeviceBlockSize(MAX_MSG_RECORD + SLACK);
    struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
    config.MaxBlocks = 2;
    config.DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_HALT;
    config.GetCapacityThreshold = ReturnsConfiguredThreshold;
    config.OnThresholdCrossed = CountThresholdCrossings;
    thresholdReturnValue = 2 * (MAX_MSG_RECORD + SLACK);
    store = SolidSyslogBlockStore_Create(&config);

    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* fills block 0 partially */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* fills block 1 partially */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* fails, sticky engages — fires once */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* fails again — must not refire */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* fails again — must not refire */

    CALLED_FUNCTION(CountThresholdCrossings, ONCE);
}

/* Given current usage well below threshold,
 * When getCapacityThreshold starts returning a value the current usage already exceeds,
 * Then onThresholdCrossed fires on the next Write. */
TEST(SolidSyslogBlockStoreCapacityThreshold, FiresWhenThresholdDropsBelowCurrentUsage)
{
    static const size_t HIGH_THRESHOLD = 1000000;
    static const size_t LOW_THRESHOLD = 1;

    CreateWithThreshold(HIGH_THRESHOLD);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    CALLED_FUNCTION(CountThresholdCrossings, NEVER); /* still well below threshold */

    thresholdReturnValue = LOW_THRESHOLD; /* threshold drops below current usage */
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);

    CALLED_FUNCTION(CountThresholdCrossings, ONCE);
}

/* Given persisted store contents already at-or-above threshold,
 * When the integrator calls SolidSyslogBlockStore_Create,
 * Then onThresholdCrossed fires once during Create. */
TEST(SolidSyslogBlockStoreCapacityThreshold, FiresOnCreateWhenResumedUsageAboveThreshold)
{
    {
        struct SolidSyslogBlockStoreConfig preConfig = MakeConfig(device);
        struct SolidSyslogStore* preStore = SolidSyslogBlockStore_Create(&preConfig);
        SolidSyslogStore_Write(preStore, TEST_DATA, TEST_DATA_LEN);
        SolidSyslogBlockStore_Destroy(preStore);
    }

    /* setup() reset CountThresholdCrossingsCallCount to 0 — any fire here is from this Create. */
    CreateWithThreshold(TEST_DATA_LEN);

    CALLED_FUNCTION(CountThresholdCrossings, ONCE);
}

/* ------------------------------------------------------------------
 * Pool — prove SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE caps live instances
 * and overflow resolves to the shared SolidSyslogNullStore. Generic
 * pool mechanics (per-probe lock, stale-handle warning) are covered
 * by SolidSyslogPoolAllocatorTest.cpp.
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP_BASE(SolidSyslogBlockStorePool, BlockDeviceTestBase)
{
    struct SolidSyslogStore* pooled[SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE] = {};
    struct SolidSyslogStore* overflow                                   = nullptr;

    void setup() override
    {
        setupBlockDeviceFakes();
    }

    void teardown() override
    {
        for (auto*& slot : pooled)
        {
            SolidSyslogBlockStore_Destroy(slot);
            slot = nullptr;
        }
        SolidSyslogBlockStore_Destroy(overflow);
        overflow = nullptr;
        teardownBlockDeviceFakes();
    }

    struct SolidSyslogStore* MakeStore()
    {
        struct SolidSyslogBlockStoreConfig config = MakeConfig(device);
        return SolidSyslogBlockStore_Create(&config);
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = MakeStore();
        }
    }
};

// clang-format on

TEST(SolidSyslogBlockStorePool, FillingPoolThenOverflowResolvesToNullStore)
{
    FillPool();

    overflow = MakeStore();

    /* Overflow resolves to the shared NullStore_Get() — distinct from every
     * pool slot, and the same singleton consumers of SolidSyslogConfig.Store
     * see when no store is wired. */
    CHECK_TEXT(overflow == SolidSyslogNullStore_Get(), "overflow did not resolve to NullStore");
    for (auto* slot : pooled)
    {
        CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");
        CHECK_TEXT(slot != SolidSyslogNullStore_Get(), "pool slot collided with NullStore singleton");
        CHECK_TEXT(overflow != slot, "Fallback handle collided with a pool slot");
    }
}

TEST(SolidSyslogBlockStorePool, UseAfterDestroyIsCrashSafeViaNullStoreVtable)
{
    /* After Destroy, the slot's vtable is overwritten with NullStore's so
     * Write/Read/etc are safe no-ops rather than NULL-fn-pointer crashes.
     * Pin the contract that Write drops, ReadNextUnsent reports nothing,
     * and the rest of the vtable doesn't crash. */
    struct SolidSyslogStore* store = MakeStore();
    pooled[0] = store; /* keep the handle live so teardown's Destroy hits the same slot — second Destroy is the
                          known-issued-handle case */
    SolidSyslogBlockStore_Destroy(store);
    pooled[0] = nullptr;

    /* Vtable now matches NullStore — Write drops, ReadNextUnsent has nothing. */
    CHECK_FALSE(SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN));
    char buf[TEST_BUF_SIZE] = {};
    size_t bytesRead = 99;
    CHECK_FALSE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    LONGS_EQUAL(0, bytesRead);
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
    SolidSyslogStore_MarkSent(store); /* no-op */
}
