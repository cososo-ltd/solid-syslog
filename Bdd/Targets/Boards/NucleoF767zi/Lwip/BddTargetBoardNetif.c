/* lwIP netif for the NUCLEO-F767ZI's Ethernet MAC - see BddTargetBoardNetif.h.
 *
 * A placeholder: it gives the stack an interface whose link never comes up,
 * so the target runs to its prompt with nothing on the wire. */

#include "BddTargetBoardNetif.h"

#include "BddTargetBoardConfig.h"

#include "lwip/etharp.h"
#include "lwip/pbuf.h"

#include <stdint.h>
#include <string.h>

enum
{
    BOARD_NETIF_MTU = 1500
};

static const uint8_t BOARD_NETIF_MAC[ETHARP_HWADDR_LEN] = {BDD_TARGET_BOARD_MAC_OCTETS};

static err_t BddTargetBoardNetif_LowLevelOutput(struct netif* netif, struct pbuf* p);

err_t BddTargetBoardNetif_Init(struct netif* netif)
{
    netif->name[0] = 'e';
    netif->name[1] = 'n';
    netif->output = etharp_output;
    netif->linkoutput = BddTargetBoardNetif_LowLevelOutput;
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    memcpy(netif->hwaddr, BOARD_NETIF_MAC, sizeof(BOARD_NETIF_MAC));
    netif->mtu = BOARD_NETIF_MTU;
    netif->flags = (u8_t) (NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP);
    return ERR_OK;
}

static err_t BddTargetBoardNetif_LowLevelOutput(struct netif* netif, struct pbuf* p)
{
    (void) netif;
    (void) p;
    return ERR_IF;
}
