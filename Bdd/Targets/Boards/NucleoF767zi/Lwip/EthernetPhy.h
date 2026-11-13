#ifndef ETHERNET_PHY_H
#define ETHERNET_PHY_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* What an Ethernet PHY says about its link, read through the registers
     * IEEE 802.3 clause 22 gives every PHY. Nothing here is particular to one
     * part, and nothing touches hardware: the caller supplies the register
     * read, which is what lets this be tested off the board. */

    enum EthernetPhyLink
    {
        ETHERNET_PHY_LINK_DOWN,
        ETHERNET_PHY_LINK_10_HALF,
        ETHERNET_PHY_LINK_10_FULL,
        ETHERNET_PHY_LINK_100_HALF,
        ETHERNET_PHY_LINK_100_FULL
    };

    /* Reads one clause-22 register. False when the PHY did not answer. */
    typedef bool (*EthernetPhyReadFunction)(void* context, uint8_t registerNumber, uint16_t* value);

    /* The link as it stands now: down, or the best speed and duplex both ends
     * advertised. A link still negotiating is down, and so is a PHY that
     * cannot be read. */
    enum EthernetPhyLink EthernetPhy_ReadLink(EthernetPhyReadFunction read, void* context);

    bool EthernetPhy_IsUp(enum EthernetPhyLink link);
    bool EthernetPhy_IsFullDuplex(enum EthernetPhyLink link);
    bool EthernetPhy_Is100Megabit(enum EthernetPhyLink link);

    /* The link in words, for a console: "down", or "up, 100 Mbit/s full duplex". */
    const char* EthernetPhy_LinkText(enum EthernetPhyLink link);

SOLIDSYSLOG_EXTERN_C_END

#endif /* ETHERNET_PHY_H */
