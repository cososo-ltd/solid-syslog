#include "EthernetPhy.h"

enum
{
    /* Basic status. Its link bit latches a drop until the register is read. */
    REGISTER_STATUS = 1,
    /* What this end advertises, and what the far end does. */
    REGISTER_ADVERTISED = 4,
    REGISTER_PARTNER = 5,

    STATUS_LINK_UP = 0x0004,
    STATUS_NEGOTIATION_COMPLETE = 0x0020,

    ABILITY_10_HALF = 0x0020,
    ABILITY_10_FULL = 0x0040,
    ABILITY_100_HALF = 0x0080,
    ABILITY_100_FULL = 0x0100
};

static inline bool EthernetPhy_LinkHasSettled(uint16_t status);
static inline enum EthernetPhyLink EthernetPhy_BestCommon(uint16_t common);

enum EthernetPhyLink EthernetPhy_ReadLink(EthernetPhyReadFunction read, void* context)
{
    uint16_t status = 0U;
    uint16_t advertised = 0U;
    uint16_t partner = 0U;
    enum EthernetPhyLink link = ETHERNET_PHY_LINK_DOWN;
    /* Twice: the first read returns, and clears, a drop latched since the last
     * look; the second says how the link is now. */
    bool wasRead = read(context, (uint8_t) REGISTER_STATUS, &status);
    wasRead = read(context, (uint8_t) REGISTER_STATUS, &status) && wasRead;
    wasRead = read(context, (uint8_t) REGISTER_ADVERTISED, &advertised) && wasRead;
    wasRead = read(context, (uint8_t) REGISTER_PARTNER, &partner) && wasRead;
    if (wasRead && EthernetPhy_LinkHasSettled(status))
    {
        link = EthernetPhy_BestCommon(advertised & partner);
    }
    return link;
}

static inline bool EthernetPhy_LinkHasSettled(uint16_t status)
{
    uint16_t settled = (uint16_t) STATUS_LINK_UP | (uint16_t) STATUS_NEGOTIATION_COMPLETE;
    return (status & settled) == settled;
}

/* The order is the one the standard resolves in: speed before duplex. */
static inline enum EthernetPhyLink EthernetPhy_BestCommon(uint16_t common)
{
    enum EthernetPhyLink link = ETHERNET_PHY_LINK_DOWN;
    if ((common & (uint16_t) ABILITY_100_FULL) != 0U)
    {
        link = ETHERNET_PHY_LINK_100_FULL;
    }
    else if ((common & (uint16_t) ABILITY_100_HALF) != 0U)
    {
        link = ETHERNET_PHY_LINK_100_HALF;
    }
    else if ((common & (uint16_t) ABILITY_10_FULL) != 0U)
    {
        link = ETHERNET_PHY_LINK_10_FULL;
    }
    else if ((common & (uint16_t) ABILITY_10_HALF) != 0U)
    {
        link = ETHERNET_PHY_LINK_10_HALF;
    }
    else
    {
        /* Nothing in common: the link cannot carry a frame. */
    }
    return link;
}

bool EthernetPhy_IsUp(enum EthernetPhyLink link)
{
    return link != ETHERNET_PHY_LINK_DOWN;
}

bool EthernetPhy_IsFullDuplex(enum EthernetPhyLink link)
{
    return (link == ETHERNET_PHY_LINK_10_FULL) || (link == ETHERNET_PHY_LINK_100_FULL);
}

bool EthernetPhy_Is100Megabit(enum EthernetPhyLink link)
{
    return (link == ETHERNET_PHY_LINK_100_HALF) || (link == ETHERNET_PHY_LINK_100_FULL);
}

const char* EthernetPhy_LinkText(enum EthernetPhyLink link)
{
    const char* text = "down";
    switch (link)
    {
        case ETHERNET_PHY_LINK_10_HALF:
            text = "up, 10 Mbit/s half duplex";
            break;
        case ETHERNET_PHY_LINK_10_FULL:
            text = "up, 10 Mbit/s full duplex";
            break;
        case ETHERNET_PHY_LINK_100_HALF:
            text = "up, 100 Mbit/s half duplex";
            break;
        case ETHERNET_PHY_LINK_100_FULL:
            text = "up, 100 Mbit/s full duplex";
            break;
        case ETHERNET_PHY_LINK_DOWN:
        default:
            break;
    }
    return text;
}
