#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogUtf8.h"

enum
{
    IPV4_HEADER_BYTES = 20,
    IPV6_HEADER_BYTES = 40,
    UDP_HEADER_BYTES = 8
};

static inline size_t UdpPayload_FindLastCodepointStart(const uint8_t* buffer, size_t length);
static inline bool UdpPayload_LastCodepointExtendsPastCut(
    const uint8_t* buffer,
    size_t length,
    size_t lastCodepointStart
);
static inline size_t UdpPayload_ExpectedSequenceLength(uint8_t startByte);

size_t SolidSyslogUdpPayload_FromMtu(size_t mtu, bool isIpv6)
{
    size_t overhead = (isIpv6 ? IPV6_HEADER_BYTES : IPV4_HEADER_BYTES) + UDP_HEADER_BYTES;
    return mtu > overhead ? mtu - overhead : 0;
}

size_t SolidSyslogUdpPayload_TrimToCodepointBoundary(const uint8_t* buffer, size_t length)
{
    if (length > 0)
    {
        size_t lastCodepointStart = UdpPayload_FindLastCodepointStart(buffer, length);
        if (UdpPayload_LastCodepointExtendsPastCut(buffer, length, lastCodepointStart))
        {
            length = lastCodepointStart;
        }
    }
    return length;
}

static inline size_t UdpPayload_FindLastCodepointStart(const uint8_t* buffer, size_t length)
{
    size_t startIndex = length - 1;
    while (startIndex > 0 && SolidSyslogUtf8_IsContinuationByte((char) buffer[startIndex]))
    {
        startIndex--;
    }
    return startIndex;
}

static inline bool UdpPayload_LastCodepointExtendsPastCut(
    const uint8_t* buffer,
    size_t length,
    size_t lastCodepointStart
)
{
    return (lastCodepointStart + UdpPayload_ExpectedSequenceLength(buffer[lastCodepointStart])) > length;
}

/* 11110xxx is the only remaining pattern — invalid bytes never reach here
 * because the formatter (S12.10) guarantees valid UTF-8 upstream. */
static inline size_t UdpPayload_ExpectedSequenceLength(uint8_t startByte)
{
    char b = (char) startByte;
    size_t length = 0;
    if (SolidSyslogUtf8_IsAsciiByte(b))
    {
        length = 1;
    }
    else if (SolidSyslogUtf8_IsTwoByteLead(b))
    {
        length = 2;
    }
    else if (SolidSyslogUtf8_IsThreeByteLead(b))
    {
        length = 3;
    }
    else
    {
        length = 4;
    }
    return length;
}
