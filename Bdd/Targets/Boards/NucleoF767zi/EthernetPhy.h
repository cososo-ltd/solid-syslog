#ifndef ETHERNET_PHY_H
#define ETHERNET_PHY_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The state of an Ethernet PHY's link, read through the registers IEEE
     * 802.3 clause 22 gives every PHY. The caller supplies the register read
     * and write, so this touches no hardware and is tested on the host. */

    enum EthernetPhyLink
    {
        ETHERNET_PHY_LINK_DOWN,
        ETHERNET_PHY_LINK_10_HALF,
        ETHERNET_PHY_LINK_10_FULL,
        ETHERNET_PHY_LINK_100_HALF,
        ETHERNET_PHY_LINK_100_FULL
    };

    /* Read and write one clause-22 register. False when the access did not
     * complete; a completed access does not mean a PHY was there to answer. */
    typedef bool (*EthernetPhyReadFunction)(void* context, uint8_t registerNumber, uint16_t* value);
    typedef bool (*EthernetPhyWriteFunction)(void* context, uint8_t registerNumber, uint16_t value);

    /* Sets the PHY to negotiate every speed and duplex up to 100 Mbit/s, and
     * resets it into that mode. A PHY latches its mode from strap pins as it
     * leaves reset, and that mode is not relied on: one has negotiation off,
     * which EthernetPhy_ReadLink reports as link down, and one powers the PHY
     * down, leaving the MAC with no reference clock. So call this before the
     * MAC is reset. False when the PHY does not come out of its reset
     * negotiating, as with an absent PHY or a failed access. */
    bool EthernetPhy_Restart(EthernetPhyWriteFunction write, EthernetPhyReadFunction read, void* context);

    /* The link now: down, or the best speed and duplex both ends advertised.
     * A link still negotiating is down, and so is a PHY that cannot be read. */
    enum EthernetPhyLink EthernetPhy_ReadLink(EthernetPhyReadFunction read, void* context);

    bool EthernetPhy_IsUp(enum EthernetPhyLink link);
    bool EthernetPhy_IsFullDuplex(enum EthernetPhyLink link);
    bool EthernetPhy_Is100Megabit(enum EthernetPhyLink link);

    /* The link in words, for a console: "down", or "up, 100 Mbit/s full duplex". */
    const char* EthernetPhy_LinkText(enum EthernetPhyLink link);

SOLIDSYSLOG_EXTERN_C_END

#endif /* ETHERNET_PHY_H */
