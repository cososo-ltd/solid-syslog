#ifndef SOLIDSYSLOGWINSOCKTCPSTREAMPRIVATE_H
#define SOLIDSYSLOGWINSOCKTCPSTREAMPRIVATE_H

#include <winsock2.h>

#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogWinsockTcpStream.h"

struct SolidSyslogWinsockTcpStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogWinsockTcpStreamConfig Config;
    SOCKET Fd;
};

void WinsockTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogWinsockTcpStreamConfig* config
);
void WinsockTcpStream_Cleanup(struct SolidSyslogStream* base);

#endif /* SOLIDSYSLOGWINSOCKTCPSTREAMPRIVATE_H */
