#include <array>
#include <cstring>
#include <initializer_list>
#include <string>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogStructuredData.h"
#include "SolidSyslogTunables.h"
#include "CppUTest/TestHarness.h"

class TEST_SolidSyslogOriginSd_EnterpriseIdContainingSpecialsIsEscaped_Test;
class TEST_SolidSyslogOriginSd_FormatIncludesDifferentEnterpriseIdFromConfig_Test;
class TEST_SolidSyslogOriginSd_FormatIncludesDifferentIpFromCallback_Test;
class TEST_SolidSyslogOriginSd_FormatIncludesEnterpriseIdFromConfig_Test;
class TEST_SolidSyslogOriginSd_FormatIncludesOneIpFromCallback_Test;
class TEST_SolidSyslogOriginSd_IpContainingSpecialsIsEscaped_Test;
struct SolidSyslogFormatter;
struct SolidSyslogStructuredData;

enum
{
    /* Worst-case fully-escaped output is 337 bytes — see
       WorstCaseFullyEscapedInputFitsPreFormattedStorage. Pre-message dispatch
       widens this further once IP params are spliced in. 512 leaves headroom. */
    TEST_BUFFER_SIZE = 512,
    MAX_FAKE_IPS = 8
};

static std::array<const char*, MAX_FAKE_IPS> fakeIps;
static size_t fakeIpCount;

static size_t FakeIpCount()
{
    return fakeIpCount;
}

static void FakeIpAt(struct SolidSyslogFormatter* f, size_t index)
{
    SolidSyslogFormatter_EscapedString(f, fakeIps.at(index), 64); // ORIGIN_IP_MAX
}

#define CHECK_ENTERPRISE_ID(expected)                                                           \
    STRCMP_EQUAL(                                                                               \
        "[origin software=\"TestSoftware\" swVersion=\"9.8.7\" enterpriseId=\"" expected "\"]", \
        SolidSyslogFormatter_AsFormattedBuffer(formatter)                                       \
    )
#define CHECK_IP(expected)                                                            \
    STRCMP_EQUAL(                                                                     \
        "[origin software=\"TestSoftware\" swVersion=\"9.8.7\" ip=\"" expected "\"]", \
        SolidSyslogFormatter_AsFormattedBuffer(formatter)                             \
    )

namespace
{
std::string repeated(char c, size_t n)
{
    std::string result(n, c);
    return result;
}

std::string escapeEach(const std::string& allSpecials)
{
    std::string out;
    for (char c : allSpecials)
    {
        out += '\\';
        out += c;
    }
    return out;
}
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogOriginSd)
{
    SolidSyslogStructuredData* sd;
    SolidSyslogOriginSdConfig config;
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    SolidSyslogFormatter* formatter;

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
        config = {};
        config.Software = "TestSoftware";
        config.SwVersion = "9.8.7";
        fakeIpCount = 0;
        sd = SolidSyslogOriginSd_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogOriginSd_Destroy(sd);
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- positional shorthand for the two existing config fields; tests assert exact output so a mix-up shows immediately
    void recreate(const char* software, const char* swVersion)
    {
        SolidSyslogOriginSd_Destroy(sd);
        config.Software = software;
        config.SwVersion = swVersion;
        sd = SolidSyslogOriginSd_Create(&config);
    }

    void useEnterpriseId(const char* enterpriseId)
    {
        SolidSyslogOriginSd_Destroy(sd);
        config.EnterpriseId = enterpriseId;
        sd = SolidSyslogOriginSd_Create(&config);
    }

    void useIps(std::initializer_list<const char*> ips)
    {
        SolidSyslogOriginSd_Destroy(sd);
        fakeIpCount = ips.size();
        size_t i = 0;
        for (const char* ip : ips)
        {
            fakeIps.at(i++) = ip;
        }
        config.GetIpCount = FakeIpCount;
        config.GetIpAt = FakeIpAt;
        sd = SolidSyslogOriginSd_Create(&config);
    }

    void format() const
    {
        SolidSyslogStructuredData_Format(sd, formatter);
    }

    void resetFormatter()
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
    }
};

// clang-format on

TEST(SolidSyslogOriginSd, CreateReturnsNonNull)
{
    CHECK(sd != nullptr);
}

TEST(SolidSyslogOriginSd, UseAfterDestroyIsCrashSafeViaNullSdVtable)
{
    /* After Destroy the slot's abstract-base vtable is the shared NullSd's, so
     * Format through the stale handle is a safe no-op rather than a NULL-fn-pointer
     * crash. NullSd.Format writes nothing into the Formatter. */
    SolidSyslogOriginSd_Destroy(sd);
    SolidSyslogStructuredData_Format(sd, formatter);
    LONGS_EQUAL(0, SolidSyslogFormatter_Length(formatter));
    sd = SolidSyslogOriginSd_Create(&config); // for teardown
}

TEST(SolidSyslogOriginSd, FormatContainsSoftwareName)
{
    format();
    CHECK(strstr(SolidSyslogFormatter_AsFormattedBuffer(formatter), "software=\"TestSoftware\"") != nullptr);
}

TEST(SolidSyslogOriginSd, FormatContainsSwVersion)
{
    format();
    CHECK(strstr(SolidSyslogFormatter_AsFormattedBuffer(formatter), "swVersion=\"9.8.7\"") != nullptr);
}

TEST(SolidSyslogOriginSd, FormatProducesCompleteOriginSd)
{
    format();
    STRCMP_EQUAL(
        "[origin software=\"TestSoftware\" swVersion=\"9.8.7\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogOriginSd, FormatAdvancesFormatterLength)
{
    LONGS_EQUAL(0, SolidSyslogFormatter_Length(formatter));
    format();
    CHECK(SolidSyslogFormatter_Length(formatter) > 0);
    LONGS_EQUAL(strlen(SolidSyslogFormatter_AsFormattedBuffer(formatter)), SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogOriginSd, DifferentValuesProduceDifferentOutput)
{
    recreate("OtherSoft", "1.2.3");
    resetFormatter();
    format();
    STRCMP_EQUAL(
        "[origin software=\"OtherSoft\" swVersion=\"1.2.3\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogOriginSd, SoftwareAtMaxLength)
{
    recreate("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijkl", "1.0");
    resetFormatter();
    format();
    CHECK(
        strstr(
            SolidSyslogFormatter_AsFormattedBuffer(formatter),
            "software=\"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijkl\""
        ) != nullptr
    );
}

TEST(SolidSyslogOriginSd, SoftwareTruncatedBeyondMaxLength)
{
    recreate("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklX", "1.0");
    resetFormatter();
    format();
    STRCMP_EQUAL(
        "[origin software=\"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijkl\" swVersion=\"1.0\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogOriginSd, SwVersionAtMaxLength)
{
    recreate("S", "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345");
    resetFormatter();
    format();
    CHECK(
        strstr(SolidSyslogFormatter_AsFormattedBuffer(formatter), "swVersion=\"ABCDEFGHIJKLMNOPQRSTUVWXYZ012345\"") !=
        nullptr
    );
}

TEST(SolidSyslogOriginSd, SwVersionTruncatedBeyondMaxLength)
{
    recreate("S", "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345X");
    resetFormatter();
    format();
    STRCMP_EQUAL(
        "[origin software=\"S\" swVersion=\"ABCDEFGHIJKLMNOPQRSTUVWXYZ012345\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogOriginSd, EmptySoftwareString)
{
    recreate("", "1.0");
    resetFormatter();
    format();
    STRCMP_EQUAL("[origin software=\"\" swVersion=\"1.0\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, EmptySwVersionString)
{
    recreate("S", "");
    resetFormatter();
    format();
    STRCMP_EQUAL("[origin software=\"S\" swVersion=\"\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, NullSoftwareOmitsTheParam)
{
    recreate(nullptr, "1.0");
    resetFormatter();
    format();
    STRCMP_EQUAL("[origin swVersion=\"1.0\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, NullSwVersionOmitsTheParam)
{
    recreate("S", nullptr);
    resetFormatter();
    format();
    STRCMP_EQUAL("[origin software=\"S\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, BothNullProducesBareOriginElement)
{
    recreate(nullptr, nullptr);
    resetFormatter();
    format();
    STRCMP_EQUAL("[origin]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, DestroyDoesNotCrash)
{
    // Covered by teardown — this test documents the intent
}

TEST(SolidSyslogOriginSd, SoftwareContainingSpecialsIsEscaped)
{
    recreate("a\"b\\c]d", "1.0");
    resetFormatter();
    format();
    STRCMP_EQUAL(
        "[origin software=\"a\\\"b\\\\c\\]d\" swVersion=\"1.0\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogOriginSd, SwVersionContainingSpecialsIsEscaped)
{
    recreate("S", "1\"2\\3]4");
    resetFormatter();
    format();
    STRCMP_EQUAL(
        "[origin software=\"S\" swVersion=\"1\\\"2\\\\3\\]4\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogOriginSd, FormatIncludesEnterpriseIdFromConfig)
{
    useEnterpriseId("1.3.6.1.4.1.12345");
    resetFormatter();
    format();
    CHECK_ENTERPRISE_ID("1.3.6.1.4.1.12345");
}

TEST(SolidSyslogOriginSd, FormatIncludesDifferentEnterpriseIdFromConfig)
{
    useEnterpriseId("1.3.6.1.4.1.99999");
    resetFormatter();
    format();
    CHECK_ENTERPRISE_ID("1.3.6.1.4.1.99999");
}

TEST(SolidSyslogOriginSd, EnterpriseIdAtMaxLength)
{
    const std::string maxId = repeated('A', 64); /* ORIGIN_ENTERPRISE_ID_MAX */
    const std::string expected =
        R"([origin software="TestSoftware" swVersion="9.8.7" enterpriseId=")" + maxId + R"("])";

    useEnterpriseId(maxId.c_str());
    resetFormatter();
    format();
    STRCMP_EQUAL(expected.c_str(), SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, EnterpriseIdTruncatedBeyondMaxLength)
{
    const std::string overlong = repeated('A', 64) + "X"; /* one past ORIGIN_ENTERPRISE_ID_MAX */
    const std::string expected =
        R"([origin software="TestSoftware" swVersion="9.8.7" enterpriseId=")" + repeated('A', 64) + R"("])";

    useEnterpriseId(overlong.c_str());
    resetFormatter();
    format();
    STRCMP_EQUAL(expected.c_str(), SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, EnterpriseIdContainingSpecialsIsEscaped)
{
    useEnterpriseId("a\"b\\c]d");
    resetFormatter();
    format();
    CHECK_ENTERPRISE_ID("a\\\"b\\\\c\\]d");
}

TEST(SolidSyslogOriginSd, WorstCaseFullyEscapedInputFitsPreFormattedStorage)
{
    const std::string software = repeated(']', 48); /* ORIGIN_SOFTWARE_MAX */
    const std::string swVersion = repeated('"', 32); /* ORIGIN_SWVERSION_MAX */
    const std::string enterpriseId = repeated('\\', 64); /* ORIGIN_ENTERPRISE_ID_MAX */
    const std::string expected = R"([origin software=")" + escapeEach(software) + R"(" swVersion=")" +
                                 escapeEach(swVersion) + R"(" enterpriseId=")" + escapeEach(enterpriseId) + R"("])";

    recreate(software.c_str(), swVersion.c_str());
    useEnterpriseId(enterpriseId.c_str());
    resetFormatter();
    format();
    STRCMP_EQUAL(expected.c_str(), SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, ZeroIpCountFromCallbackOmitsIpParams)
{
    useIps({});
    resetFormatter();
    format();
    STRCMP_EQUAL(
        "[origin software=\"TestSoftware\" swVersion=\"9.8.7\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogOriginSd, FormatIncludesOneIpFromCallback)
{
    useIps({"192.0.2.1"});
    resetFormatter();
    format();
    CHECK_IP("192.0.2.1");
}

TEST(SolidSyslogOriginSd, FormatIncludesDifferentIpFromCallback)
{
    useIps({"203.0.113.5"});
    resetFormatter();
    format();
    CHECK_IP("203.0.113.5");
}

TEST(SolidSyslogOriginSd, FormatIncludesMultipleIpsFromCallback)
{
    useIps({"192.0.2.1", "192.0.2.2", "10.0.0.1"});
    resetFormatter();
    format();
    STRCMP_EQUAL(
        "[origin software=\"TestSoftware\" swVersion=\"9.8.7\" ip=\"192.0.2.1\" ip=\"192.0.2.2\" ip=\"10.0.0.1\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogOriginSd, IpAtMaxLength)
{
    const std::string maxIp = repeated('a', 64); /* ORIGIN_IP_MAX */
    const std::string expected = R"([origin software="TestSoftware" swVersion="9.8.7" ip=")" + maxIp + R"("])";

    useIps({maxIp.c_str()});
    resetFormatter();
    format();
    STRCMP_EQUAL(expected.c_str(), SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, IpContainingSpecialsIsEscaped)
{
    useIps({"a\"b\\c]d"});
    resetFormatter();
    format();
    CHECK_IP("a\\\"b\\\\c\\]d");
}

TEST(SolidSyslogOriginSd, GetIpCountSetButGetIpAtNullOmitsIpParams)
{
    SolidSyslogOriginSd_Destroy(sd);
    config.GetIpCount = FakeIpCount;
    config.GetIpAt = nullptr;
    sd = SolidSyslogOriginSd_Create(&config);
    resetFormatter();
    format();
    STRCMP_EQUAL(
        "[origin software=\"TestSoftware\" swVersion=\"9.8.7\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogOriginSd, GetIpAtSetButGetIpCountNullOmitsIpParams)
{
    SolidSyslogOriginSd_Destroy(sd);
    config.GetIpCount = nullptr;
    config.GetIpAt = FakeIpAt;
    sd = SolidSyslogOriginSd_Create(&config);
    resetFormatter();
    format();
    STRCMP_EQUAL(
        "[origin software=\"TestSoftware\" swVersion=\"9.8.7\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogOriginSd, AllFourParamsTogether)
{
    useEnterpriseId("1.3.6.1.4.1.12345");
    useIps({"192.0.2.1"});
    resetFormatter();
    format();
    STRCMP_EQUAL(
        "[origin software=\"TestSoftware\" swVersion=\"9.8.7\" enterpriseId=\"1.3.6.1.4.1.12345\" ip=\"192.0.2.1\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogOriginSd, EnterpriseIdOnlyNoStaticNoIps)
{
    config.Software = nullptr;
    config.SwVersion = nullptr;
    useEnterpriseId("1.3.6.1.4.1.12345");
    resetFormatter();
    format();
    STRCMP_EQUAL("[origin enterpriseId=\"1.3.6.1.4.1.12345\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, IpsOnlyNoStatic)
{
    config.Software = nullptr;
    config.SwVersion = nullptr;
    useIps({"192.0.2.1"});
    resetFormatter();
    format();
    STRCMP_EQUAL("[origin ip=\"192.0.2.1\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, EnterpriseIdAndIpsNoSoftwareSwVersion)
{
    config.Software = nullptr;
    config.SwVersion = nullptr;
    useEnterpriseId("1.3.6.1.4.1.12345");
    useIps({"192.0.2.1"});
    resetFormatter();
    format();
    STRCMP_EQUAL(
        "[origin enterpriseId=\"1.3.6.1.4.1.12345\" ip=\"192.0.2.1\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

// Pool tests — prove SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE caps live instances
// and overflow falls back to the shared SolidSyslogNullSd.

// clang-format off
TEST_GROUP(SolidSyslogOriginSdPool)
{
    SolidSyslogOriginSdConfig config{};
    struct SolidSyslogStructuredData* pooled[SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE] = {};
    struct SolidSyslogStructuredData* overflow                                  = nullptr;

    void setup() override
    {
        config.Software = "TestSoftware";
        config.SwVersion = "1.0.0";
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogOriginSd_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogOriginSd_Destroy(overflow);
        }
    }

    struct SolidSyslogStructuredData* MakeSd()
    {
        return SolidSyslogOriginSd_Create(&config);
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = MakeSd();
        }
    }
};

// clang-format on

TEST(SolidSyslogOriginSdPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = MakeSd();

    CHECK_TEXT(overflow != nullptr, "Fallback handle was nullptr");
    for (auto* slot : pooled)
    {
        CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");
        CHECK_TEXT(overflow != slot, "Fallback handle collided with a pool slot");
    }
}
