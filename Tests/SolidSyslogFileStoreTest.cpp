#include "CppUTest/TestHarness.h"
#include "SolidSyslogFileStore.h"
#include "SolidSyslogCrc16Policy.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslog.h"
#include "FileFake.h"

#include <cstring>

static const char* const TEST_PATH_PREFIX = "/tmp/test_store";
static const char* const TEST_DATA        = "hello";
static const size_t      TEST_DATA_LEN    = 5;

enum
{
    TEST_BUF_SIZE = SOLIDSYSLOG_MAX_MESSAGE_SIZE,
    SENTINEL      = 'Z',
    /* Mirrors the private RECORD_OVERHEAD in SolidSyslogFileStore.c:
     * MAGIC_SIZE(2) + RECORD_LENGTH_SIZE(2) + SENT_FLAG_SIZE(1). */
    TEST_RECORD_OVERHEAD  = 5,
    TEST_RECORDS_PER_FILE = 2,
    /* Sized to fit TEST_RECORDS_PER_FILE worst-case records — the worst
     * case being max-size data plus max-integrity bytes. Auto-adapts
     * when SOLIDSYSLOG_MAX_MESSAGE_SIZE or the integrity policy bound
     * are tuned. */
    TEST_MAX_FILE_SIZE = TEST_RECORDS_PER_FILE * (SOLIDSYSLOG_MAX_MESSAGE_SIZE + TEST_RECORD_OVERHEAD + SOLIDSYSLOG_MAX_INTEGRITY_SIZE),
    TEST_MAX_FILES     = 2
};

static const struct SolidSyslogFileStoreConfig DEFAULT_CONFIG = {
    nullptr, nullptr, TEST_PATH_PREFIX, TEST_MAX_FILE_SIZE, TEST_MAX_FILES, SOLIDSYSLOG_DISCARD_OLDEST, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};

/* Single backing slab reused across tests — tests run serially and Destroy
 * resets the store, so one storage instance is sufficient. */
static SolidSyslogFileStoreStorage storeStorage = {};

static struct SolidSyslogFileStoreConfig MakeConfig(struct SolidSyslogFile* file)
{
    struct SolidSyslogFileStoreConfig config = DEFAULT_CONFIG;
    config.readFile                          = file;
    config.writeFile                         = file;
    return config;
}

/* ------------------------------------------------------------------
 * Basic operations
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP(SolidSyslogFileStore)
{
    struct FileFakeStorage storage = {};
    struct SolidSyslogFile* file = nullptr;
    struct SolidSyslogStore*   store   = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        file = FileFake_Create(&storage);
        struct SolidSyslogFileStoreConfig config = MakeConfig(file);
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = SolidSyslogFileStore_Create(&storeStorage, &config);
    }

    void teardown() override
    {
        SolidSyslogFileStore_Destroy(store);
        FileFake_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogFileStore, CreateReturnsNonNull)
{
    CHECK_TRUE(store != nullptr);
}

TEST(SolidSyslogFileStore, CreatesFileWithSequence00)
{
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/test_store00.log"));
}

TEST(SolidSyslogFileStore, HasUnsentReturnsFalseOnEmpty)
{
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStore, WriteReturnsTrue)
{
    CHECK_TRUE(SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN));
}

TEST(SolidSyslogFileStore, HasUnsentReturnsTrueAfterWrite)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStore, ReadNextUnsentReturnsTrueAfterWrite)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char   buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
}

TEST(SolidSyslogFileStore, ReadNextUnsentReturnsWrittenData)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char   buf[TEST_BUF_SIZE] = {};
    size_t bytesRead          = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL(TEST_DATA, buf, TEST_DATA_LEN);
}

TEST(SolidSyslogFileStore, ReadNextUnsentReturnsByteCount)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char   buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    LONGS_EQUAL(TEST_DATA_LEN, bytesRead);
}

TEST(SolidSyslogFileStore, ReadNextUnsentReturnsFalseOnEmpty)
{
    char   buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    CHECK_FALSE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
}

TEST(SolidSyslogFileStore, ReadNextUnsentSetsZeroBytesOnEmpty)
{
    char   buf[TEST_BUF_SIZE];
    size_t bytesRead = 99;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    LONGS_EQUAL(0, bytesRead);
}

TEST(SolidSyslogFileStore, ReadDoesNotWriteBeyondDataLength)
{
    char buf[TEST_BUF_SIZE];
    memset(buf, SENTINEL, sizeof(buf));

    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);

    MEMCMP_EQUAL(TEST_DATA, buf, TEST_DATA_LEN);
    BYTES_EQUAL(SENTINEL, buf[TEST_DATA_LEN]);
}

TEST(SolidSyslogFileStore, ReadTruncatesWhenBufferTooSmall)
{
    const char* longMessage = "hello world";

    enum
    {
        SMALL_BUF_SIZE = 5
    };

    SolidSyslogStore_Write(store, longMessage, strlen(longMessage));

    char   buf[SMALL_BUF_SIZE] = {};
    size_t bytesRead           = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);

    LONGS_EQUAL(SMALL_BUF_SIZE, bytesRead);
    MEMCMP_EQUAL("hello", buf, SMALL_BUF_SIZE);
}

TEST(SolidSyslogFileStore, MarkSentThenHasUnsentReturnsFalse)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char   buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStore, MarkSentWithoutReadDoesNotCrash)
{
    SolidSyslogStore_MarkSent(store);
}

TEST(SolidSyslogFileStore, HasUnsentFalseAfterAllSent)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char   buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStore, WriteAfterDrainWorks)
{
    SolidSyslogStore_Write(store, "first", strlen("first"));
    char   buf[TEST_BUF_SIZE] = {};
    size_t bytesRead          = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    SolidSyslogStore_Write(store, "second", strlen("second"));
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("second", buf, strlen("second"));
}

TEST(SolidSyslogFileStore, TwoWritesFirstReadReturnsFirst)
{
    SolidSyslogStore_Write(store, "first", strlen("first"));
    SolidSyslogStore_Write(store, "second", strlen("second"));
    char   buf[TEST_BUF_SIZE] = {};
    size_t bytesRead          = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("first", buf, strlen("first"));
}

TEST(SolidSyslogFileStore, AfterMarkFirstReadReturnsSecond)
{
    SolidSyslogStore_Write(store, "first", strlen("first"));
    SolidSyslogStore_Write(store, "second", strlen("second"));
    char   buf[TEST_BUF_SIZE] = {};
    size_t bytesRead          = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("second", buf, strlen("second"));
    LONGS_EQUAL(strlen("second"), bytesRead);
}

TEST(SolidSyslogFileStore, FiveWritesDrainAllInOrder)
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
        char   buf[TEST_BUF_SIZE] = {};
        size_t bytesRead          = 0;
        CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
        MEMCMP_EQUAL(expected, buf, MESSAGE_LEN);
        SolidSyslogStore_MarkSent(store);
    }

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

/* ------------------------------------------------------------------
 * Resume from existing file
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP(SolidSyslogFileStoreResume)
{
    struct FileFakeStorage storage = {};
    struct SolidSyslogFile* file = nullptr;
    struct SolidSyslogStore*   store   = nullptr;

    void setup() override
    {
        file = FileFake_Create(&storage);
    }

    void teardown() override
    {
        SolidSyslogFileStore_Destroy(store);
        FileFake_Destroy();
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- total and markedSent have distinct semantics
    void WritePreviousSession(int total, int markedSent)
    {
        struct SolidSyslogFileStoreConfig config = MakeConfig(file);
        // cppcheck-suppress unreadVariable -- used by WriteMessages/DrainMessages; cppcheck does not model CppUTest macros
        store = SolidSyslogFileStore_Create(&storeStorage, &config);
        WriteMessages(total);
        DrainMessages(markedSent);
        SolidSyslogFileStore_Destroy(store);

        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = SolidSyslogFileStore_Create(&storeStorage, &config);
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

TEST(SolidSyslogFileStoreResume, HasUnsentAfterResume)
{
    WritePreviousSession(3, 1);
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreResume, ReadReturnsFirstUnsent)
{
    WritePreviousSession(3, 1);
    char   buf[TEST_BUF_SIZE] = {};
    size_t bytesRead          = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("msg1", buf, strlen("msg1"));
}

TEST(SolidSyslogFileStoreResume, DrainsRemainingUnsent)
{
    WritePreviousSession(3, 1);
    char   buf[TEST_BUF_SIZE] = {};
    size_t bytesRead          = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("msg1", buf, strlen("msg1"));
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    MEMCMP_EQUAL("msg2", buf, strlen("msg2"));
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreResume, AllSentReturnsNoUnsent)
{
    WritePreviousSession(3, 3);
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreResume, EmptyFileReturnsNoUnsent)
{
    WritePreviousSession(0, 0);
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreResume, CanWriteNewMessagesAfterResume)
{
    WritePreviousSession(2, 1);
    char   buf[TEST_BUF_SIZE] = {};
    size_t bytesRead          = 0;

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
TEST_GROUP(SolidSyslogFileStoreDestroy)
{
    struct FileFakeStorage storage = {};
    struct SolidSyslogFile* file = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        file = FileFake_Create(&storage);
    }

    void teardown() override
    {
        FileFake_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogFileStoreDestroy, DestroyClosesFile)
{
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    struct SolidSyslogStore*          store  = SolidSyslogFileStore_Create(&storeStorage, &config);
    CHECK_TRUE(SolidSyslogFile_IsOpen(file));
    SolidSyslogFileStore_Destroy(store);
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogFileStoreDestroy, DoubleDestroyDoesNotCrash)
{
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    struct SolidSyslogStore*          store  = SolidSyslogFileStore_Create(&storeStorage, &config);
    SolidSyslogFileStore_Destroy(store);
    SolidSyslogFileStore_Destroy(store);
}

/* ------------------------------------------------------------------
 * Config validation
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP(SolidSyslogFileStoreConfig)
{
    struct FileFakeStorage storage = {};
    struct SolidSyslogFile* file = nullptr;
    struct SolidSyslogStore*   store   = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        file = FileFake_Create(&storage);
    }

    void teardown() override
    {
        SolidSyslogFileStore_Destroy(store);
        FileFake_Destroy();
    }

    void CreateWithMaxFiles(size_t maxFiles)
    {
        struct SolidSyslogFileStoreConfig config = MakeConfig(file);
        config.maxFiles = maxFiles;
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = SolidSyslogFileStore_Create(&storeStorage, &config);
    }

    void CreateWithMaxFileSize(size_t maxFileSize)
    {
        struct SolidSyslogFileStoreConfig config = MakeConfig(file);
        config.maxFileSize = maxFileSize;
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = SolidSyslogFileStore_Create(&storeStorage, &config);
    }

    void CreateWithPathPrefix(const char* prefix)
    {
        struct SolidSyslogFileStoreConfig config = MakeConfig(file);
        config.pathPrefix = prefix;
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = SolidSyslogFileStore_Create(&storeStorage, &config);
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

TEST(SolidSyslogFileStoreConfig, MaxFilesZeroClampedToMinimum)
{
    CreateWithMaxFiles(0);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogFileStoreConfig, MaxFilesOneClampedToMinimum)
{
    CreateWithMaxFiles(1);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogFileStoreConfig, MaxFilesHundredClampedToMaximum)
{
    CreateWithMaxFiles(100);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogFileStoreConfig, MaxFileSizeZeroClampedToMinimum)
{
    CreateWithMaxFileSize(0);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogFileStoreConfig, MaxFileSizeOneClampedToMinimum)
{
    CreateWithMaxFileSize(1);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogFileStoreConfig, FilenameExactlyAtMaxPath)
{
    /* MAX_PATH_SIZE=128, suffix "00.log"=6, null=1, so max prefix=121 chars */
    char prefix[122];
    memset(prefix, 'A', 121);
    prefix[121] = '\0';

    CreateWithPathPrefix(prefix);

    char expected[129];
    memset(expected, 'A', 121);
    memcpy(expected + 121, "00.log", 7);

    CHECK_TRUE(SolidSyslogFile_Exists(file, expected));
}

TEST(SolidSyslogFileStoreConfig, FilenameTruncatedWhenPrefixTooLong)
{
    /* MAX_PATH_SIZE=128. A 127-char prefix leaves 1 byte for digits and
       suffix. FormatFilename must not write past the buffer — prior to
       the fix, SolidSyslogFormat_Character wrote 2 bytes unconditionally
       (char + null), overflowing filename[128]. ASan detects this. */
    char prefix[128];
    memset(prefix, 'B', 127);
    prefix[127] = '\0';

    CreateWithPathPrefix(prefix);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogFileStoreConfig, NullSecurityPolicyDefaultsToNoOp)
{
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    config.securityPolicy                    = nullptr;
    store                                    = SolidSyslogFileStore_Create(&storeStorage, &config);
    VerifyWriteAndReadBack();
}

TEST(SolidSyslogFileStoreConfig, OversizedSecurityPolicyLeavesNoIntegrityGap)
{
    struct SolidSyslogSecurityPolicy oversizedPolicy = {
        SOLIDSYSLOG_MAX_INTEGRITY_SIZE + 1,
        nullptr,
        nullptr,
    };
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    config.securityPolicy                    = &oversizedPolicy;
    store                                    = SolidSyslogFileStore_Create(&storeStorage, &config);

    const char   body[]  = "HELLO WORLD";
    const size_t bodyLen = sizeof(body) - 1;
    CHECK_TRUE(SolidSyslogStore_Write(store, body, bodyLen));

    const size_t RECORD_HEADER = 4;
    const size_t SENT_FLAG     = 1;
    LONGS_EQUAL(RECORD_HEADER + bodyLen + SENT_FLAG, FileFake_FileSize());
    MEMCMP_EQUAL(body, static_cast<const uint8_t*>(FileFake_FileContent()) + RECORD_HEADER, bodyLen);
}

/* ------------------------------------------------------------------
 * Error paths
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP(SolidSyslogFileStoreErrors)
{
    struct FileFakeStorage storage = {};
    struct SolidSyslogFile* file = nullptr;
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogStore*   store   = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        file = FileFake_Create(&storage);
    }

    void teardown() override
    {
        SolidSyslogFileStore_Destroy(store);
        FileFake_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogFileStoreErrors, OpenFailureStillReturnsNonNull)
{
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    FileFake_FailNextOpen();
    store = SolidSyslogFileStore_Create(&storeStorage, &config);
    CHECK_TRUE(store != nullptr);
}

TEST(SolidSyslogFileStoreErrors, WriteReturnsFalseWhenNotOpen)
{
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    FileFake_FailNextOpen();
    store = SolidSyslogFileStore_Create(&storeStorage, &config);
    CHECK_FALSE(SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN));
}

TEST(SolidSyslogFileStoreErrors, WriteReturnsFalseOnWriteFailure)
{
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    store                                    = SolidSyslogFileStore_Create(&storeStorage, &config);
    FileFake_FailNextWrite();
    CHECK_FALSE(SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN));
}

TEST(SolidSyslogFileStoreErrors, ReadReturnsFalseOnReadFailure)
{
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    store                                    = SolidSyslogFileStore_Create(&storeStorage, &config);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    FileFake_FailNextRead();

    char   buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    CHECK_FALSE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    LONGS_EQUAL(0, bytesRead);
}

TEST(SolidSyslogFileStoreErrors, HasUnsentReturnsFalseWhenNotOpen)
{
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    FileFake_FailNextOpen();
    store = SolidSyslogFileStore_Create(&storeStorage, &config);
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreErrors, MarkSentDoesNotAdvanceWhenWriteFails)
{
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    store                                    = SolidSyslogFileStore_Create(&storeStorage, &config);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);

    char   buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);

    FileFake_FailNextWrite();
    SolidSyslogStore_MarkSent(store);

    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
}

/* ------------------------------------------------------------------
 * File rotation
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP(SolidSyslogFileStoreRotation)
{
    static const size_t RECORD_OVERHEAD    = 5; /* 2 (magic) + 2 (length) + 1 (sent flag) */
    static const size_t ONE_MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + RECORD_OVERHEAD;

    struct FileFakeStorage readStorage = {};
    struct FileFakeStorage writeStorage = {};
    struct SolidSyslogFile* readFile = nullptr;
    struct SolidSyslogFile* writeFile = nullptr;
    struct SolidSyslogStore* store = nullptr;
    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};

    void setup() override
    {
        readFile = FileFake_Create(&readStorage);
        writeFile = FileFake_Create(&writeStorage);
        memset(maxMsg, 'A', sizeof(maxMsg));
    }

    void teardown() override
    {
        SolidSyslogFileStore_Destroy(store);
        FileFake_Destroy();
    }

    void CreateWithMaxFileSize(size_t maxFileSize, enum SolidSyslogDiscardPolicy policy = SOLIDSYSLOG_DISCARD_OLDEST,
                               size_t maxFiles = 2,
                               SolidSyslogStoreFullCallback onStoreFull = nullptr, void* storeFullContext = nullptr)
    {
        struct SolidSyslogFileStoreConfig config = DEFAULT_CONFIG;
        config.readFile          = readFile;
        config.writeFile         = writeFile;
        config.maxFileSize       = maxFileSize;
        config.maxFiles          = maxFiles;
        config.discardPolicy     = policy;
        config.onStoreFull       = onStoreFull;
        config.storeFullContext  = storeFullContext;
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = SolidSyslogFileStore_Create(&storeStorage, &config);
    }

    void WriteMaxMsg()
    {
        SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg));
    }
};

// clang-format on

TEST(SolidSyslogFileStoreRotation, WriteRotatesToNewFileWhenFull)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg();
    WriteMaxMsg();
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store01.log"));
}

TEST(SolidSyslogFileStoreRotation, WriteDoesNotRotateWhenFileHasSpace)
{
    CreateWithMaxFileSize(2 * ONE_MAX_MSG_RECORD);
    WriteMaxMsg();
    WriteMaxMsg();
    CHECK_FALSE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store01.log"));
}

TEST(SolidSyslogFileStoreRotation, HasUnsentReturnsTrueAfterRotation)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg();
    WriteMaxMsg();
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreRotation, ReadReturnsFirstFileAfterRotation)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg));

    WriteMaxMsg(); /* rotates to file 01 */

    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead                         = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);

    LONGS_EQUAL(sizeof(firstMsg), bytesRead);
    BYTES_EQUAL('B', buf[0]);
}

TEST(SolidSyslogFileStoreRotation, MarkSentAdvancesReadToSecondFile)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg));

    WriteMaxMsg(); /* rotates to file 01 */

    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead                         = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);

    LONGS_EQUAL(SOLIDSYSLOG_MAX_MESSAGE_SIZE, bytesRead);
    BYTES_EQUAL('A', buf[0]);
}

TEST(SolidSyslogFileStoreRotation, FullDrainAcrossTwoFilesHasUnsentFalse)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg();
    WriteMaxMsg();

    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t bytesRead = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreRotation, DiscardOldestDeletesOldestFileWhenAtMaxFiles)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* file 00 */
    WriteMaxMsg(); /* file 01 — now at maxFiles=2 */
    WriteMaxMsg(); /* file 02 — must discard 00 */

    CHECK_FALSE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store00.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store02.log"));
}

TEST(SolidSyslogFileStoreRotation, DiscardOldestSurvivingDataIsReadable)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg)); /* file 00 */

    char secondMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(secondMsg, 'C', sizeof(secondMsg));
    SolidSyslogStore_Write(store, secondMsg, sizeof(secondMsg)); /* file 01 */

    WriteMaxMsg(); /* file 02 — discards 00 */

    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead                         = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);

    LONGS_EQUAL(SOLIDSYSLOG_MAX_MESSAGE_SIZE, bytesRead);
    BYTES_EQUAL('C', buf[0]);
}

TEST(SolidSyslogFileStoreRotation, DiscardOldestDrainYieldsOnlySurvivingRecords)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg)); /* file 00 — will be discarded */

    char secondMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(secondMsg, 'C', sizeof(secondMsg));
    SolidSyslogStore_Write(store, secondMsg, sizeof(secondMsg)); /* file 01 — survives */

    WriteMaxMsg(); /* file 02 — triggers discard of file 00 */

    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead                         = 0;

    /* First record should be from surviving file 01, not discarded file 00 */
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    BYTES_EQUAL('C', buf[0]);
    SolidSyslogStore_MarkSent(store);

    /* Second record from file 02 */
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    BYTES_EQUAL('A', buf[0]);
    SolidSyslogStore_MarkSent(store);

    /* No more records */
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreRotation, DiscardNewestReturnsFalseWhenAtMaxFiles)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_DISCARD_NEWEST);
    WriteMaxMsg(); /* file 00 */
    WriteMaxMsg(); /* file 01 — now at maxFiles=2 */

    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
}

static bool storeFullCallbackInvoked;

static void StoreFullCallback(void* context)
{
    (void) context;
    storeFullCallbackInvoked = true;
}

TEST(SolidSyslogFileStoreRotation, HaltInvokesCallbackWhenStoreFull)
{
    storeFullCallbackInvoked = false;
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_HALT, 2, StoreFullCallback);

    WriteMaxMsg(); /* file 00 */
    WriteMaxMsg(); /* file 01 — now at maxFiles=2 */

    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
    CHECK_TRUE(storeFullCallbackInvoked);
}

TEST(SolidSyslogFileStoreRotation, HaltWithNullCallbackDoesNotCrash)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_HALT);

    WriteMaxMsg(); /* file 00 */
    WriteMaxMsg(); /* file 01 — now at maxFiles=2 */

    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
}

TEST(SolidSyslogFileStoreRotation, HaltSetsIsHaltedTrue)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_HALT);

    WriteMaxMsg(); /* file 00 */
    WriteMaxMsg(); /* file 01 — now at maxFiles=2 */

    CHECK_FALSE(SolidSyslogStore_IsHalted(store));
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* triggers halt */
    CHECK_TRUE(SolidSyslogStore_IsHalted(store));
}

TEST(SolidSyslogFileStoreRotation, DiscardNewestDoesNotInvokeCallback)
{
    storeFullCallbackInvoked = false;
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_DISCARD_NEWEST, 2, StoreFullCallback);

    WriteMaxMsg(); /* file 00 */
    WriteMaxMsg(); /* file 01 — now at maxFiles=2 */

    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
    CHECK_FALSE(storeFullCallbackInvoked);
}

static int storeFullCallbackCount;

static void CountStoreFullInvocations(void* context)
{
    (void) context;
    storeFullCallbackCount++;
}

TEST(SolidSyslogFileStoreRotation, HaltOnStoreFullFiresOncePerRisingEdge)
{
    storeFullCallbackCount = 0;
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_HALT, 2, CountStoreFullInvocations);

    WriteMaxMsg(); /* file 00 */
    WriteMaxMsg(); /* file 01 — now at maxFiles=2 */

    /* Three consecutive failed Writes — callback must fire on the first only. */
    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));

    LONGS_EQUAL(1, storeFullCallbackCount);
}

static void* storeFullCallbackContext;

static void StoreFullCallbackCapturingContext(void* context)
{
    storeFullCallbackContext = context;
}

/* Given the integrator wires storeFullContext at config time,
 * When onStoreFull fires,
 * Then the callback receives the configured context pointer unchanged. */
TEST(SolidSyslogFileStoreRotation, OnStoreFullReceivesConfiguredContext)
{
    int sentinel             = 0;
    storeFullCallbackContext = nullptr;

    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_HALT, 2, StoreFullCallbackCapturingContext, &sentinel);

    WriteMaxMsg(); /* file 00 */
    WriteMaxMsg(); /* file 01 — at maxFiles */

    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* triggers halt callback */

    POINTERS_EQUAL(&sentinel, storeFullCallbackContext);
}

TEST(SolidSyslogFileStoreRotation, ResumeHasUnsentWhenMultipleFilesExist)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* file 00 */
    WriteMaxMsg(); /* file 01 */
    SolidSyslogFileStore_Destroy(store);

    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreRotation, ResumeDrainsAcrossFilesInOrder)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg)); /* file 00 */

    WriteMaxMsg(); /* file 01 — 'A' */
    SolidSyslogFileStore_Destroy(store);

    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);

    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead                         = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    BYTES_EQUAL('B', buf[0]);
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    BYTES_EQUAL('A', buf[0]);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreRotation, ResumeContinuesWritingToCorrectFile)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* file 00 */
    SolidSyslogFileStore_Destroy(store);

    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* should rotate to file 01, not overwrite 00 */

    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store00.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store01.log"));
}

TEST(SolidSyslogFileStoreRotation, ResumeWithMultipleFilesCanWriteNewMessage)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* file 00 */
    WriteMaxMsg(); /* file 01 */
    SolidSyslogFileStore_Destroy(store);

    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);

    char newMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(newMsg, 'N', sizeof(newMsg));
    CHECK_TRUE(SolidSyslogStore_Write(store, newMsg, sizeof(newMsg)));

    /* Should have rotated to file 02 — file 01 was full */
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store02.log"));
}

TEST(SolidSyslogFileStoreRotation, ResumeWriteAppendsToPartiallyFilledWriteFile)
{
    static const size_t TWO_MAX_MSG_RECORDS = 2 * ONE_MAX_MSG_RECORD;

    CreateWithMaxFileSize(TWO_MAX_MSG_RECORDS);
    WriteMaxMsg(); /* file 00, record 1 */
    WriteMaxMsg(); /* file 00, record 2 — file 00 full */
    WriteMaxMsg(); /* file 01, record 1 — file 01 partially filled */
    SolidSyslogFileStore_Destroy(store);

    CreateWithMaxFileSize(TWO_MAX_MSG_RECORDS);

    /* Write should append to file 01, not rotate */
    CHECK_TRUE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));
    CHECK_FALSE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store02.log"));
}

TEST(SolidSyslogFileStoreRotation, SequenceWrapsFrom99To00)
{
    /* Pre-seed file 99 so the scan finds it as the write file */
    SolidSyslogFile_Open(writeFile, "/tmp/test_store99.log");
    SolidSyslogFile_Close(writeFile);

    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* fills file 99 */
    WriteMaxMsg(); /* should wrap to file 00 */

    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store00.log"));
}

TEST(SolidSyslogFileStoreRotation, WriteAfterDrainRotatesToNextFile)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg();

    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));

    /* Drained file still occupies space — next write rotates */
    WriteMaxMsg();
    CHECK_TRUE(SolidSyslogStore_HasUnsent(store));
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store01.log"));
}

TEST(SolidSyslogFileStoreRotation, DestroyClosesBothHandles)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);
    WriteMaxMsg(); /* file 00 */
    WriteMaxMsg(); /* file 01 — read on 00, write on 01 */
    SolidSyslogFileStore_Destroy(store);

    CHECK_FALSE(SolidSyslogFile_IsOpen(readFile));
    CHECK_FALSE(SolidSyslogFile_IsOpen(writeFile));
}

TEST(SolidSyslogFileStoreRotation, MixedMessageSizesDrainCorrectlyAcrossFiles)
{
    static const size_t SHORT_LEN = 7;

    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);

    char shortMsg[SHORT_LEN];
    memset(shortMsg, 'S', SHORT_LEN);
    SolidSyslogStore_Write(store, shortMsg, SHORT_LEN); /* file 00 — small record */

    WriteMaxMsg(); /* file 01 — max record */

    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead                         = 0;

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

TEST(SolidSyslogFileStoreRotation, ContinuousDiscardWithoutReadingSurvivorsCorrect)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);

    /* Write 5 messages across 5 files — maxFiles=2 means 3 are discarded */
    char msgs[5][SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    for (int i = 0; i < 5; i++)
    {
        memset(msgs[i], 'A' + i, sizeof(msgs[i]));               // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        SolidSyslogStore_Write(store, msgs[i], sizeof(msgs[i])); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /* Only files 03 and 04 should survive */
    CHECK_FALSE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store00.log"));
    CHECK_FALSE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store01.log"));
    CHECK_FALSE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store02.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store03.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store04.log"));

    /* Drain — should get msg3 ('D') then msg4 ('E') */
    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead                         = 0;

    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    BYTES_EQUAL('D', buf[0]);
    SolidSyslogStore_MarkSent(store);

    memset(buf, 0, sizeof(buf));
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    BYTES_EQUAL('E', buf[0]);
    SolidSyslogStore_MarkSent(store);

    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreRotation, MaxFilesAtUpperLimit)
{
    enum
    {
        MAX_FILES = 99
    };

    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD, SOLIDSYSLOG_DISCARD_OLDEST, MAX_FILES);

    /* Fill all 99 files */
    for (int i = 0; i < MAX_FILES; i++)
    {
        WriteMaxMsg();
    }

    /* All 99 files should exist (00–98) */
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store00.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store98.log"));
    CHECK_FALSE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store99.log"));

    /* One more write — should discard file 00 and create file 99 */
    WriteMaxMsg();

    CHECK_FALSE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store00.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store99.log"));
}

TEST(SolidSyslogFileStoreRotation, MultipleRecordsPerFileDrainAcrossRotation)
{
    static const size_t TWO_MAX_MSG_RECORDS = 2 * ONE_MAX_MSG_RECORD;

    CreateWithMaxFileSize(TWO_MAX_MSG_RECORDS);

    char msg0[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(msg0, 'X', sizeof(msg0));
    SolidSyslogStore_Write(store, msg0, sizeof(msg0)); /* file 00, record 1 */

    char msg1[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(msg1, 'Y', sizeof(msg1));
    SolidSyslogStore_Write(store, msg1, sizeof(msg1)); /* file 00, record 2 */

    WriteMaxMsg(); /* file 01, record 1 — 'A' */

    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead                         = 0;

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

/* ------------------------------------------------------------------
 * Integrity (SecurityPolicy integration)
 * ----------------------------------------------------------------*/

enum
{
    INTEGRITY_REGION_MAX = 2 + 2 + SOLIDSYSLOG_MAX_MESSAGE_SIZE /* magic + length + body */
};

static bool     computeIntegrityCalled;
static uint8_t  computeIntegrityData[INTEGRITY_REGION_MAX];
static uint16_t computeIntegrityLength;

// NOLINTNEXTLINE(readability-non-const-parameter) -- matches SecurityPolicy vtable signature
static void SpyComputeIntegrity(const uint8_t* data, uint16_t length, uint8_t* integrityOut)
{
    (void) integrityOut;
    computeIntegrityCalled = true;
    computeIntegrityLength = length;
    memcpy(computeIntegrityData, data, length);
}

static bool     verifyIntegrityCalled;
static uint8_t  verifyIntegrityData[INTEGRITY_REGION_MAX];
static uint16_t verifyIntegrityLength;

static bool SpyVerifyIntegrity(const uint8_t* data, uint16_t length, const uint8_t* integrityIn)
{
    (void) integrityIn;
    verifyIntegrityCalled = true;
    verifyIntegrityLength = length;
    memcpy(verifyIntegrityData, data, length);
    return true;
}

static struct SolidSyslogSecurityPolicy spyPolicy = {
    0,
    SpyComputeIntegrity,
    SpyVerifyIntegrity,
};

// clang-format off
TEST_GROUP(SolidSyslogFileStoreIntegrity)
{
    struct FileFakeStorage storage = {};
    struct SolidSyslogFile* file = nullptr;
    struct SolidSyslogStore* store = nullptr;

    void setup() override
    {
        file = FileFake_Create(&storage);
        computeIntegrityCalled  = false;
        computeIntegrityLength  = 0;
        memset(computeIntegrityData, 0, sizeof(computeIntegrityData));
        verifyIntegrityCalled   = false;
        verifyIntegrityLength   = 0;
        memset(verifyIntegrityData, 0, sizeof(verifyIntegrityData));

        struct SolidSyslogFileStoreConfig config = DEFAULT_CONFIG;
        config.readFile       = file;
        config.writeFile      = file;
        config.securityPolicy = &spyPolicy;
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = SolidSyslogFileStore_Create(&storeStorage, &config);
    }

    void teardown() override
    {
        SolidSyslogFileStore_Destroy(store);
        FileFake_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogFileStoreIntegrity, WriteCallsComputeIntegrity)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    CHECK_TRUE(computeIntegrityCalled);
}

TEST(SolidSyslogFileStoreIntegrity, ComputeIntegrityReceivesIntegrityRegion)
{
    enum
    {
        MAGIC_SIZE  = 2,
        LENGTH_SIZE = 2,
        REGION_SIZE = MAGIC_SIZE + LENGTH_SIZE + TEST_DATA_LEN
    };

    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    LONGS_EQUAL(REGION_SIZE, computeIntegrityLength);

    /* verify magic bytes */
    BYTES_EQUAL(0xA5, computeIntegrityData[0]);
    BYTES_EQUAL(0x5A, computeIntegrityData[1]);

    /* verify body is included */
    MEMCMP_EQUAL(TEST_DATA, computeIntegrityData + MAGIC_SIZE + LENGTH_SIZE, TEST_DATA_LEN);
}

TEST(SolidSyslogFileStoreIntegrity, ReadCallsVerifyIntegrity)
{
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char   buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    CHECK_TRUE(verifyIntegrityCalled);
}

TEST(SolidSyslogFileStoreIntegrity, VerifyIntegrityReceivesIntegrityRegion)
{
    enum
    {
        MAGIC_SIZE  = 2,
        LENGTH_SIZE = 2,
        REGION_SIZE = MAGIC_SIZE + LENGTH_SIZE + TEST_DATA_LEN
    };

    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    char   buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead);
    LONGS_EQUAL(REGION_SIZE, verifyIntegrityLength);

    /* verify magic bytes */
    BYTES_EQUAL(0xA5, verifyIntegrityData[0]);
    BYTES_EQUAL(0x5A, verifyIntegrityData[1]);

    /* verify body is included */
    MEMCMP_EQUAL(TEST_DATA, verifyIntegrityData + MAGIC_SIZE + LENGTH_SIZE, TEST_DATA_LEN);
}

/* ------------------------------------------------------------------
 * Corruption detection
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP(SolidSyslogFileStoreCorruption)
{
    struct FileFakeStorage storage = {};
    struct SolidSyslogFile* file = nullptr;
    struct SolidSyslogStore* store = nullptr;

    void setup() override
    {
        file = FileFake_Create(&storage);
    }

    void teardown() override
    {
        SolidSyslogFileStore_Destroy(store);
        FileFake_Destroy();
    }

    void WriteRawBytes(const char* path, const void* data, size_t size) const
    {
        SolidSyslogFile_Open(file, path);
        SolidSyslogFile_Write(file, data, size);
        SolidSyslogFile_Close(file);
    }

    void CreateStore()
    {
        struct SolidSyslogFileStoreConfig config = MakeConfig(file);
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = SolidSyslogFileStore_Create(&storeStorage, &config);
    }
};

// clang-format on

TEST(SolidSyslogFileStoreCorruption, TruncatedMagicHasNoUnsent)
{
    uint8_t oneByte = 0xA5;
    WriteRawBytes("/tmp/test_store00.log", &oneByte, 1);
    CreateStore();
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreCorruption, BadMagicHasNoUnsent)
{
    uint8_t badMagic[] = {0x00, 0x00, 0x05, 0x00, 'h', 'e', 'l', 'l', 'o', 0xFF};
    WriteRawBytes("/tmp/test_store00.log", badMagic, sizeof(badMagic));
    CreateStore();
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreCorruption, TruncatedLengthHasNoUnsent)
{
    uint8_t truncatedHeader[] = {0xA5, 0x5A, 0x05};
    WriteRawBytes("/tmp/test_store00.log", truncatedHeader, sizeof(truncatedHeader));
    CreateStore();
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreCorruption, TruncatedBodyHasNoUnsent)
{
    /* valid magic + length=5, but only 2 bytes of body, no integrity or sent flag */
    uint8_t truncatedBody[] = {0xA5, 0x5A, 0x05, 0x00, 'h', 'e'};
    WriteRawBytes("/tmp/test_store00.log", truncatedBody, sizeof(truncatedBody));
    CreateStore();
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));
}

TEST(SolidSyslogFileStoreCorruption, ValidRecordBeforeCorruptionIsReadable)
{
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    config.securityPolicy                    = SolidSyslogCrc16Policy_Create();
    store                                    = SolidSyslogFileStore_Create(&storeStorage, &config);
    SolidSyslogStore_Write(store, "first", 5);
    SolidSyslogStore_Write(store, "second", 6);
    SolidSyslogFileStore_Destroy(store);

    /* Corrupt the second record's body */
    enum
    {
        MAGIC                     = 2,
        LENGTH                    = 2,
        FIRST_BODY                = 5,
        CRC                       = 2,
        SENT                      = 1,
        SECOND_RECORD_BODY_OFFSET = MAGIC + LENGTH + FIRST_BODY + CRC + SENT + MAGIC + LENGTH
    };

    SolidSyslogFile_Open(file, "/tmp/test_store00.log");
    uint8_t corrupt = 0xFF;
    SolidSyslogFile_SeekTo(file, SECOND_RECORD_BODY_OFFSET);
    SolidSyslogFile_Write(file, &corrupt, 1);
    SolidSyslogFile_Close(file);

    /* Re-open: first record is valid, second is corrupt */
    store                     = SolidSyslogFileStore_Create(&storeStorage, &config);
    char   buf[TEST_BUF_SIZE] = {};
    size_t bytesRead          = 0;

    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    LONGS_EQUAL(5, bytesRead);
    MEMCMP_EQUAL("first", buf, 5);
}

TEST(SolidSyslogFileStoreCorruption, IntegrityFailureReadReturnsFalse)
{
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    config.securityPolicy                    = SolidSyslogCrc16Policy_Create();
    store                                    = SolidSyslogFileStore_Create(&storeStorage, &config);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    SolidSyslogFileStore_Destroy(store);

    /* Corrupt one byte of the body in the stored record */
    SolidSyslogFile_Open(file, "/tmp/test_store00.log");
    uint8_t corrupt = 0xFF;
    SolidSyslogFile_SeekTo(file, 4); /* offset past magic(2) + length(2) */
    SolidSyslogFile_Write(file, &corrupt, 1);
    SolidSyslogFile_Close(file);

    store = SolidSyslogFileStore_Create(&storeStorage, &config);
    char   buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    CHECK_FALSE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
}

TEST(SolidSyslogFileStoreCorruption, InvalidLengthReadReturnsFalse)
{
    /* Write many records to make the file large enough that a bogus length
     * doesn't hit EOF — the length check must reject it explicitly */
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    store                                    = SolidSyslogFileStore_Create(&storeStorage, &config);

    char largeMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(largeMsg, 'X', sizeof(largeMsg));
    SolidSyslogStore_Write(store, largeMsg, sizeof(largeMsg));
    SolidSyslogStore_Write(store, largeMsg, sizeof(largeMsg));
    SolidSyslogFileStore_Destroy(store);

    /* Overwrite the length field of the first record (bytes 2-3) */
    SolidSyslogFile_Open(file, "/tmp/test_store00.log");
    uint16_t badLength = SOLIDSYSLOG_MAX_MESSAGE_SIZE + 1;
    SolidSyslogFile_SeekTo(file, 2);
    SolidSyslogFile_Write(file, &badLength, 2);
    SolidSyslogFile_Close(file);

    store = SolidSyslogFileStore_Create(&storeStorage, &config);
    char   buf[TEST_BUF_SIZE];
    size_t bytesRead = 0;
    CHECK_FALSE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
}

/* ------------------------------------------------------------------
 * Corruption recovery
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP(SolidSyslogFileStoreCorruptionRecovery)
{
    static const size_t RECORD_OVERHEAD    = 7; /* 2 (magic) + 2 (length) + 2 (crc) + 1 (sent) */
    static const size_t ONE_MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + RECORD_OVERHEAD;

    struct FileFakeStorage readStorage = {};
    struct FileFakeStorage writeStorage = {};
    struct SolidSyslogFile* readFile = nullptr;
    struct SolidSyslogFile* writeFile = nullptr;
    struct SolidSyslogStore* store = nullptr;
    struct SolidSyslogSecurityPolicy* policy = nullptr;
    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};

    void setup() override
    {
        readFile = FileFake_Create(&readStorage);
        writeFile = FileFake_Create(&writeStorage);
        policy = SolidSyslogCrc16Policy_Create();
        memset(maxMsg, 'A', sizeof(maxMsg));
    }

    void teardown() override
    {
        SolidSyslogFileStore_Destroy(store);
        FileFake_Destroy();
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- maxFileSize and maxFiles have distinct semantics
    void CreateWithMaxFileSize(size_t maxFileSize, size_t maxFiles = 2)
    {
        struct SolidSyslogFileStoreConfig config = DEFAULT_CONFIG;
        config.readFile        = readFile;
        config.writeFile       = writeFile;
        config.maxFileSize     = maxFileSize;
        config.maxFiles        = maxFiles;
        config.securityPolicy  = policy;
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        store = SolidSyslogFileStore_Create(&storeStorage, &config);
    }

    void WriteMaxMsg()
    {
        SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg));
    }

    void CorruptFirstRecordBody(const char* path) const
    {
        SolidSyslogFile_Open(writeFile, path);
        uint8_t corrupt = 0xFF;
        SolidSyslogFile_SeekTo(writeFile, 4);
        SolidSyslogFile_Write(writeFile, &corrupt, 1);
        SolidSyslogFile_Close(writeFile);
    }
};

// clang-format on

TEST(SolidSyslogFileStoreCorruptionRecovery, ReadSkipsCorruptOlderFileToNextFile)
{
    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);

    char firstMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(firstMsg, 'B', sizeof(firstMsg));
    SolidSyslogStore_Write(store, firstMsg, sizeof(firstMsg)); /* file 00 */

    WriteMaxMsg(); /* file 01 */
    SolidSyslogFileStore_Destroy(store);

    CorruptFirstRecordBody("/tmp/test_store00.log");

    CreateWithMaxFileSize(ONE_MAX_MSG_RECORD);

    char   buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead                         = 0;
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    LONGS_EQUAL(SOLIDSYSLOG_MAX_MESSAGE_SIZE, bytesRead);
    BYTES_EQUAL('A', buf[0]);
}

TEST(SolidSyslogFileStoreCorruptionRecovery, CorruptWriteFileRotatesOnNextWrite)
{
    /* Use a file size that fits two records — the first write leaves space,
     * so rotation on the second write proves corruption forced it */
    static const size_t TWO_MAX_MSG_RECORDS = 2 * ONE_MAX_MSG_RECORD;

    CreateWithMaxFileSize(TWO_MAX_MSG_RECORDS);
    WriteMaxMsg(); /* file 00 — partially filled */
    SolidSyslogFileStore_Destroy(store);

    CorruptFirstRecordBody("/tmp/test_store00.log");

    CreateWithMaxFileSize(TWO_MAX_MSG_RECORDS);

    /* File 00 has space but is corrupt — write should rotate to file 01 */
    char newMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(newMsg, 'N', sizeof(newMsg));
    CHECK_TRUE(SolidSyslogStore_Write(store, newMsg, sizeof(newMsg)));
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/test_store01.log"));
}

/* ------------------------------------------------------------------
 * Capacity getters
 * ----------------------------------------------------------------*/

// clang-format off
TEST_GROUP(SolidSyslogFileStoreCapacity)
{
    static const size_t ONE_MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + TEST_RECORD_OVERHEAD;

    struct FileFakeStorage storage = {};
    struct SolidSyslogFile* file = nullptr;
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogStore* store = nullptr;
    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};

    void setup() override
    {
        file = FileFake_Create(&storage);
        memset(maxMsg, 'A', sizeof(maxMsg));
    }

    void teardown() override
    {
        if (store != nullptr) { SolidSyslogFileStore_Destroy(store); }
        FileFake_Destroy();
    }

    void CreateDefault()
    {
        struct SolidSyslogFileStoreConfig config = MakeConfig(file);
        store = SolidSyslogFileStore_Create(&storeStorage, &config);
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- maxFileSize is a byte size, maxFiles is a count; distinct semantics
    void CreateWithCapacity(size_t maxFileSize, size_t maxFiles,
                            enum SolidSyslogDiscardPolicy policy = SOLIDSYSLOG_DISCARD_OLDEST)
    {
        struct SolidSyslogFileStoreConfig config = MakeConfig(file);
        config.maxFileSize   = maxFileSize;
        config.maxFiles      = maxFiles;
        config.discardPolicy = policy;
        store                = SolidSyslogFileStore_Create(&storeStorage, &config);
    }

    void WriteMaxMsg()
    {
        SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg));
    }
};

// clang-format on

/* Given maxFiles × maxFileSize configured,
 * When GetTotalBytes is queried,
 * Then it returns the product. */
TEST(SolidSyslogFileStoreCapacity, GetTotalBytesReturnsMaxFilesTimesMaxFileSize)
{
    CreateDefault();
    LONGS_EQUAL(TEST_MAX_FILES * TEST_MAX_FILE_SIZE, SolidSyslogStore_GetTotalBytes(store));
}

TEST(SolidSyslogFileStoreCapacity, GetTotalBytesScalesWithConfig)
{
    CreateWithCapacity(10000, 3);
    LONGS_EQUAL(3 * 10000, SolidSyslogStore_GetTotalBytes(store));
}

/* Given an empty store,
 * When GetUsedBytes is queried,
 * Then it returns 0. */
TEST(SolidSyslogFileStoreCapacity, GetUsedBytesIsZeroOnEmptyStore)
{
    CreateDefault();
    LONGS_EQUAL(0, SolidSyslogStore_GetUsedBytes(store));
}

/* Given an empty store,
 * When records totalling X bytes are written,
 * Then GetUsedBytes returns X (including record overhead). */
TEST(SolidSyslogFileStoreCapacity, GetUsedBytesTracksOneWrite)
{
    CreateDefault();
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    LONGS_EQUAL(TEST_DATA_LEN + TEST_RECORD_OVERHEAD, SolidSyslogStore_GetUsedBytes(store));
}

TEST(SolidSyslogFileStoreCapacity, GetUsedBytesCountsClosedBlocksAtFullSize)
{
    /* Tight maxFileSize so a single max-msg record fills a block; second write
     * rotates. Closed block contributes maxFileSize regardless of slack. */
    CreateWithCapacity(ONE_MAX_MSG_RECORD, 3);
    WriteMaxMsg(); /* block 0 fills */
    WriteMaxMsg(); /* rotates to block 1 */

    LONGS_EQUAL(ONE_MAX_MSG_RECORD + ONE_MAX_MSG_RECORD, SolidSyslogStore_GetUsedBytes(store));
}

/* Given a full store with SOLIDSYSLOG_DISCARD_OLDEST,
 * When the oldest block is discarded,
 * Then GetUsedBytes drops by one block size. */
TEST(SolidSyslogFileStoreCapacity, GetUsedBytesDropsOnDiscardOldest)
{
    CreateWithCapacity(ONE_MAX_MSG_RECORD, 2);
    WriteMaxMsg(); /* block 0 full */
    WriteMaxMsg(); /* block 1 full, at maxFiles */

    LONGS_EQUAL(2 * ONE_MAX_MSG_RECORD, SolidSyslogStore_GetUsedBytes(store));

    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN); /* rotates to block 2, discards block 0 */

    /* 1 closed block (block 1) + active block holds one small record. */
    LONGS_EQUAL(ONE_MAX_MSG_RECORD + TEST_DATA_LEN + TEST_RECORD_OVERHEAD, SolidSyslogStore_GetUsedBytes(store));
}

/* Given a store at capacity with SOLIDSYSLOG_HALT,
 * When a Write fails for size,
 * Then GetUsedBytes returns total even when the active block has slack. */
TEST(SolidSyslogFileStoreCapacity, GetUsedBytesIsStickyAtTotalAfterSizeFailure)
{
    /* maxFileSize larger than one max-msg record so the active block has slack. */
    static const size_t SLACK = 100;
    CreateWithCapacity(ONE_MAX_MSG_RECORD + SLACK, 2, SOLIDSYSLOG_HALT);
    WriteMaxMsg(); /* block 0: SLACK bytes slack */
    WriteMaxMsg(); /* block 1: SLACK bytes slack, at maxFiles */

    /* The next write needs to rotate but can't (HALT, at maxFiles) — fails for size. */
    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)));

    /* Sticky: GetUsedBytes returns total even though the active blocks have slack. */
    LONGS_EQUAL(SolidSyslogStore_GetTotalBytes(store), SolidSyslogStore_GetUsedBytes(store));
}

/* ------------------------------------------------------------------
 * Capacity threshold alert (S05.09)
 * ----------------------------------------------------------------*/

static int    thresholdCallbackCount;
static size_t thresholdReturnValue;

static size_t ReturnsConfiguredThreshold(void* context)
{
    (void) context;
    return thresholdReturnValue;
}

static void CountThresholdCrossings(void* context)
{
    (void) context;
    thresholdCallbackCount++;
}

// clang-format off
TEST_GROUP(SolidSyslogFileStoreCapacityThreshold)
{
    struct FileFakeStorage storage = {};
    struct SolidSyslogFile* file = nullptr;
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogStore* store = nullptr;

    void setup() override
    {
        file                   = FileFake_Create(&storage);
        thresholdCallbackCount = 0;
        thresholdReturnValue   = 0;
    }

    void teardown() override
    {
        if (store != nullptr) { SolidSyslogFileStore_Destroy(store); }
        FileFake_Destroy();
    }

    void CreateWithThreshold(size_t threshold)
    {
        struct SolidSyslogFileStoreConfig config = MakeConfig(file);
        config.getCapacityThreshold              = ReturnsConfiguredThreshold;
        config.onThresholdCrossed                = CountThresholdCrossings;
        thresholdReturnValue                     = threshold;
        store                                    = SolidSyslogFileStore_Create(&storeStorage, &config);
    }
};

// clang-format on

/* Given a threshold below the size of a single record's overhead,
 * When a write makes used-bytes cross the threshold,
 * Then onThresholdCrossed fires. */
TEST(SolidSyslogFileStoreCapacityThreshold, FiresOnRisingEdgeCrossing)
{
    CreateWithThreshold(TEST_DATA_LEN);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    LONGS_EQUAL(1, thresholdCallbackCount);
}

/* Given usage already above threshold,
 * When subsequent writes keep usage above threshold,
 * Then onThresholdCrossed fires only on the rising edge. */
TEST(SolidSyslogFileStoreCapacityThreshold, FiresOnceWhileUsageStaysAbove)
{
    CreateWithThreshold(TEST_DATA_LEN);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN); /* crosses */
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN); /* still above */
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN); /* still above */
    LONGS_EQUAL(1, thresholdCallbackCount);
}

/* Given DISCARD_OLDEST and a threshold sitting in the last block,
 * When writes cross the threshold, a discard drops usage below it, then writes cross again,
 * Then onThresholdCrossed fires twice. */
TEST(SolidSyslogFileStoreCapacityThreshold, ReArmsAfterFallingEdgeOnDiscardOldest)
{
    static const size_t MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + TEST_RECORD_OVERHEAD;
    static const size_t TWO_RECORDS    = 2 * MAX_MSG_RECORD;

    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(maxMsg, 'A', sizeof(maxMsg));

    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    config.maxFileSize                       = TWO_RECORDS;
    config.maxFiles                          = 2;
    config.discardPolicy                     = SOLIDSYSLOG_DISCARD_OLDEST;
    config.getCapacityThreshold              = ReturnsConfiguredThreshold;
    config.onThresholdCrossed                = CountThresholdCrossings;
    /* Threshold sits between 3 and 4 records: 4-records crosses, 3-records is below. */
    thresholdReturnValue = (3 * MAX_MSG_RECORD) + 1;
    store                = SolidSyslogFileStore_Create(&storeStorage, &config);

    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* block 0: 1 record */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* block 0: 2 records (full) */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* rotate; block 1: 1 record (3 total) */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* block 1: 2 records (4 total) — fires */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* rotate+discard block 0 → 3 records (below) */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* block 2: 2 records (4 total) — fires again */

    LONGS_EQUAL(2, thresholdCallbackCount);
}

/* Given getCapacityThreshold returns 0,
 * When usage rises arbitrarily,
 * Then onThresholdCrossed never fires. */
TEST(SolidSyslogFileStoreCapacityThreshold, DoesNotFireWhenThresholdIsZero)
{
    CreateWithThreshold(0);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    LONGS_EQUAL(0, thresholdCallbackCount);
}

/* Given getCapacityThreshold is NULL but onThresholdCrossed is configured,
 * When usage rises arbitrarily,
 * Then onThresholdCrossed never fires (and the library does not deref a NULL function). */
TEST(SolidSyslogFileStoreCapacityThreshold, DoesNotFireWhenThresholdFunctionIsNull)
{
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    config.getCapacityThreshold              = nullptr;
    config.onThresholdCrossed                = CountThresholdCrossings;
    store                                    = SolidSyslogFileStore_Create(&storeStorage, &config);

    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);

    LONGS_EQUAL(0, thresholdCallbackCount);
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
TEST(SolidSyslogFileStoreCapacityThreshold, ContextIsPassedToBothCallbacks)
{
    int sentinel                             = 0;
    capturedThresholdFunctionContext         = nullptr;
    capturedThresholdCallbackContext         = nullptr;
    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    config.getCapacityThreshold              = CaptureThresholdFunctionContext;
    config.onThresholdCrossed                = CaptureThresholdCallbackContext;
    config.thresholdContext                  = &sentinel;
    thresholdReturnValue                     = TEST_DATA_LEN;
    store                                    = SolidSyslogFileStore_Create(&storeStorage, &config);

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

/* Given threshold = 100% (total bytes) and SOLIDSYSLOG_HALT,
 * When a Write fails for size and engages the sticky 100% bit,
 * Then onThresholdCrossed fires before onStoreFull on that same Write. */
TEST(SolidSyslogFileStoreCapacityThreshold, AtFullCapacityWithHaltThresholdFiresBeforeStoreFull)
{
    static const size_t MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + TEST_RECORD_OVERHEAD;
    static const size_t SLACK          = 100;

    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(maxMsg, 'A', sizeof(maxMsg));

    nextFireOrder      = 0;
    thresholdFireOrder = 0;
    storeFullFireOrder = 0;

    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    config.maxFileSize                       = MAX_MSG_RECORD + SLACK;
    config.maxFiles                          = 2;
    config.discardPolicy                     = SOLIDSYSLOG_HALT;
    config.onStoreFull                       = RecordStoreFullFireOrder;
    config.getCapacityThreshold              = ReturnsConfiguredThreshold;
    config.onThresholdCrossed                = RecordThresholdFireOrder;
    /* Threshold = total: only the sticky-100% engagement on a failed Write reaches it. */
    thresholdReturnValue = 2 * (MAX_MSG_RECORD + SLACK);
    store                = SolidSyslogFileStore_Create(&storeStorage, &config);

    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg));              /* block 0 partially full */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg));              /* rotate; block 1 partially full */
    CHECK_FALSE(SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg))); /* HALT: fails, sticky engages */

    CHECK_TRUE(thresholdFireOrder > 0);
    CHECK_TRUE(storeFullFireOrder > 0);
    CHECK_TRUE(thresholdFireOrder < storeFullFireOrder);
}

/* Given a failed Write has already engaged the sticky 100% bit,
 * When subsequent Writes also fail for size,
 * Then onThresholdCrossed does not fire again. */
TEST(SolidSyslogFileStoreCapacityThreshold, StickyHundredPercentDoesNotRefireThreshold)
{
    static const size_t MAX_MSG_RECORD = SOLIDSYSLOG_MAX_MESSAGE_SIZE + TEST_RECORD_OVERHEAD;
    static const size_t SLACK          = 100;

    char maxMsg[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(maxMsg, 'A', sizeof(maxMsg));

    struct SolidSyslogFileStoreConfig config = MakeConfig(file);
    config.maxFileSize                       = MAX_MSG_RECORD + SLACK;
    config.maxFiles                          = 2;
    config.discardPolicy                     = SOLIDSYSLOG_HALT;
    config.getCapacityThreshold              = ReturnsConfiguredThreshold;
    config.onThresholdCrossed                = CountThresholdCrossings;
    thresholdReturnValue                     = 2 * (MAX_MSG_RECORD + SLACK);
    store                                    = SolidSyslogFileStore_Create(&storeStorage, &config);

    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* fills block 0 partially */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* fills block 1 partially */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* fails, sticky engages — fires once */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* fails again — must not refire */
    SolidSyslogStore_Write(store, maxMsg, sizeof(maxMsg)); /* fails again — must not refire */

    LONGS_EQUAL(1, thresholdCallbackCount);
}

/* Given current usage well below threshold,
 * When getCapacityThreshold starts returning a value the current usage already exceeds,
 * Then onThresholdCrossed fires on the next Write. */
TEST(SolidSyslogFileStoreCapacityThreshold, FiresWhenThresholdDropsBelowCurrentUsage)
{
    static const size_t HIGH_THRESHOLD = 1000000;
    static const size_t LOW_THRESHOLD  = 1;

    CreateWithThreshold(HIGH_THRESHOLD);
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);
    LONGS_EQUAL(0, thresholdCallbackCount); /* still well below threshold */

    thresholdReturnValue = LOW_THRESHOLD; /* threshold drops below current usage */
    SolidSyslogStore_Write(store, TEST_DATA, TEST_DATA_LEN);

    LONGS_EQUAL(1, thresholdCallbackCount);
}

/* Given persisted store contents already at-or-above threshold,
 * When the integrator calls SolidSyslogFileStore_Create,
 * Then onThresholdCrossed fires once during Create. */
TEST(SolidSyslogFileStoreCapacityThreshold, FiresOnCreateWhenResumedUsageAboveThreshold)
{
    {
        struct SolidSyslogFileStoreConfig preConfig = MakeConfig(file);
        struct SolidSyslogStore*          preStore  = SolidSyslogFileStore_Create(&storeStorage, &preConfig);
        SolidSyslogStore_Write(preStore, TEST_DATA, TEST_DATA_LEN);
        SolidSyslogFileStore_Destroy(preStore);
    }

    /* setup() reset thresholdCallbackCount to 0 — any fire here is from this Create. */
    CreateWithThreshold(TEST_DATA_LEN);

    LONGS_EQUAL(1, thresholdCallbackCount);
}
