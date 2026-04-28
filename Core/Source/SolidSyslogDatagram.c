#include "SolidSyslogDatagramDefinition.h"

bool SolidSyslogDatagram_Open(struct SolidSyslogDatagram* datagram)
{
    return datagram->Open(datagram);
}

enum SolidSyslogDatagramSendResult SolidSyslogDatagram_SendTo(struct SolidSyslogDatagram* datagram, const void* buffer, size_t size,
                                                              const struct SolidSyslogAddress* addr)
{
    return datagram->SendTo(datagram, buffer, size, addr);
}

size_t SolidSyslogDatagram_MaxPayload(struct SolidSyslogDatagram* datagram)
{
    return datagram->MaxPayload(datagram);
}

void SolidSyslogDatagram_Close(struct SolidSyslogDatagram* datagram)
{
    datagram->Close(datagram);
}
