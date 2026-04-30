#include "CppUTest/TestHarness.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogStructuredData.h"

#include <cstring>
#include <string>

enum
{
    TEST_BUFFER_SIZE = 256
};

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

std::string originSdWith(const std::string& software, const std::string& swVersion)
{
    return "[origin software=\"" + software + "\" swVersion=\"" + swVersion + "\"]";
}
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogOriginSd)
{
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogStructuredData* sd;
    SolidSyslogOriginSdConfig config;
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogFormatter* formatter;

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
        config = {};
        config.software = "TestSoftware";
        config.swVersion = "9.8.7";
        sd = SolidSyslogOriginSd_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogOriginSd_Destroy();
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- positional shorthand for the two existing config fields; tests assert exact output so a mix-up shows immediately
    void recreate(const char* software, const char* swVersion)
    {
        SolidSyslogOriginSd_Destroy();
        config.software = software;
        config.swVersion = swVersion;
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
    STRCMP_EQUAL("[origin software=\"TestSoftware\" swVersion=\"9.8.7\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
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
    STRCMP_EQUAL("[origin software=\"OtherSoft\" swVersion=\"1.2.3\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, SoftwareAtMaxLength)
{
    recreate("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijkl", "1.0");
    resetFormatter();
    format();
    CHECK(strstr(SolidSyslogFormatter_AsFormattedBuffer(formatter), "software=\"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijkl\"") != nullptr);
}

TEST(SolidSyslogOriginSd, SoftwareTruncatedBeyondMaxLength)
{
    recreate("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklX", "1.0");
    resetFormatter();
    format();
    STRCMP_EQUAL("[origin software=\"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijkl\" swVersion=\"1.0\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, SwVersionAtMaxLength)
{
    recreate("S", "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345");
    resetFormatter();
    format();
    CHECK(strstr(SolidSyslogFormatter_AsFormattedBuffer(formatter), "swVersion=\"ABCDEFGHIJKLMNOPQRSTUVWXYZ012345\"") != nullptr);
}

TEST(SolidSyslogOriginSd, SwVersionTruncatedBeyondMaxLength)
{
    recreate("S", "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345X");
    resetFormatter();
    format();
    STRCMP_EQUAL("[origin software=\"S\" swVersion=\"ABCDEFGHIJKLMNOPQRSTUVWXYZ012345\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
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
    STRCMP_EQUAL("[origin software=\"a\\\"b\\\\c\\]d\" swVersion=\"1.0\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, SwVersionContainingSpecialsIsEscaped)
{
    recreate("S", "1\"2\\3]4");
    resetFormatter();
    format();
    STRCMP_EQUAL("[origin software=\"S\" swVersion=\"1\\\"2\\\\3\\]4\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogOriginSd, WorstCaseFullyEscapedInputFitsPreFormattedStorage)
{
    const std::string software  = repeated(']', 48); /* ORIGIN_SOFTWARE_MAX */
    const std::string swVersion = repeated('"', 32); /* ORIGIN_SWVERSION_MAX */

    recreate(software.c_str(), swVersion.c_str());
    resetFormatter();
    format();
    STRCMP_EQUAL(originSdWith(escapeEach(software), escapeEach(swVersion)).c_str(), SolidSyslogFormatter_AsFormattedBuffer(formatter));
}
