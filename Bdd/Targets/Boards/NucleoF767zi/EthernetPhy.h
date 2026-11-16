#ifndef ETHERNET_PHY_H
#define ETHERNET_PHY_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* What an Ethernet PHY says about its link, read through the registers
     * IEEE 802.3 clause 22 gives every PHY. Nothing here is particular to one
     * part, and nothing touches hardware: the caller supplies the register
     * read and write, which is what lets this be tested off the board. */

    enum EthernetPhyLink
    {
        ETHERNET_PHY_LINK_DOWN,
        ETHERNET_PHY_LINK_10_HALF,
        ETHERNET_PHY_LINK_10_FULL,
        ETHERNET_PHY_LINK_100_HALF,
        ETHERNET_PHY_LINK_100_FULL
    };

    /* Reads one clause-22 register. False when the access did not complete,
     * which is not whether a PHY was there to take it. */
    typedef bool (*EthernetPhyReadFunction)(void* context, uint8_t registerNumber, uint16_t* value);

    /* Writes one clause-22 register. False when the access did not complete,
     * which is not whether a PHY was there to take it. */
    typedef bool (*EthernetPhyWriteFunction)(void* context, uint8_t registerNumber, uint16_t value);

    /* Puts the PHY in the mode that negotiates every speed and duplex it has
     * up to 100 Mbit/s, and resets it into that mode. A PHY takes its mode
     * from strap pins as it leaves reset, and what it took is not relied on:
     * one mode has negotiation off, which EthernetPhy_ReadLink reads as a link
     * that is down, and one has the PHY powered down, with no reference clock
     * for the MAC. So this comes before the MAC is reset. False when the PHY
     * does not come out of its reset negotiating, which is also what an
     * absent PHY and a failed access give. */
    bool EthernetPhy_Restart(EthernetPhyWriteFunction write, EthernetPhyReadFunction read, void* context);

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
