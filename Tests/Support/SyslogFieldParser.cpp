#include "SyslogFieldParser.h"

#include <string>

const char UTF8_BOM[] = "\xEF\xBB\xBF";
static const auto UTF8_BOM_LENGTH = sizeof(UTF8_BOM) - 1;

static std::string::size_type SkipSdata(const std::string& s, std::string::size_type pos)
{
    if (pos < s.size() && s[pos] == '[')
    {
        while (pos < s.size() && s[pos] == '[')
        {
            pos = s.find(']', pos);
            if (pos == std::string::npos)
            {
                return std::string::npos;
            }
            pos++;
        }
        return pos;
    }
    auto end = s.find(' ', pos);
    return end == std::string::npos ? s.size() : end;
}

static std::string::size_type FindFieldStart(const std::string& s, int n)
{
    std::string::size_type pos = 0U;
    for (int i = 0; i < n; i++)
    {
        if (i == SYSLOG_FIELD_SDATA)
        {
            pos = SkipSdata(s, pos);
        }
        else
        {
            pos = s.find(' ', pos);
        }
        if (pos == std::string::npos)
        {
            return std::string::npos;
        }
        if (s[pos] == ' ')
        {
            pos++;
        }
    }
    return pos;
}

std::string SyslogField(const char* buffer, int n)
{
    std::string s(buffer);
    std::string::size_type pos = FindFieldStart(s, n);
    if (pos == std::string::npos)
    {
        return {};
    }

    std::string::size_type end = (n == SYSLOG_FIELD_SDATA) ? SkipSdata(s, pos) : s.find(' ', pos);
    return s.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

static std::string::size_type SyslogMsgStart(const std::string& s)
{
    std::string::size_type pos = FindFieldStart(s, SYSLOG_FIELD_SDATA);
    if (pos == std::string::npos)
    {
        return std::string::npos;
    }
    pos = SkipSdata(s, pos);
    if (pos == std::string::npos || pos >= s.size())
    {
        return std::string::npos;
    }
    if (s[pos] == ' ')
    {
        pos++;
    }
    return pos;
}

bool SyslogMsgHasBom(const char* buffer)
{
    std::string s(buffer);
    std::string::size_type pos = SyslogMsgStart(s);
    return (pos != std::string::npos) && (s.compare(pos, UTF8_BOM_LENGTH, UTF8_BOM) == 0);
}

std::string SyslogMsg(const char* buffer)
{
    std::string s(buffer);
    std::string::size_type pos = SyslogMsgStart(s);
    if (pos == std::string::npos)
    {
        return {};
    }
    if (s.compare(pos, UTF8_BOM_LENGTH, UTF8_BOM) == 0)
    {
        pos += UTF8_BOM_LENGTH;
    }
    return s.substr(pos);
}
