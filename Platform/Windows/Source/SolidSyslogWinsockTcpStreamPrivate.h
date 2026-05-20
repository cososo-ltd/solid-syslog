#ifndef SOLIDSYSLOGWINSOCKTCPSTREAMPRIVATE_H
#define SOLIDSYSLOGWINSOCKTCPSTREAMPRIVATE_H

#include <winsock2.h>

#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogWinsockTcpStream
{
    struct SolidSyslogStream Base;
    SOCKET Fd;
};

void WinsockTcpStream_Initialise(struct SolidSyslogStream* base);
void WinsockTcpStream_Cleanup(struct SolidSyslogStream* base);

#endif /* SOLIDSYSLOGWINSOCKTCPSTREAMPRIVATE_H */
