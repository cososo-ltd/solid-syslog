/* The NUCLEO-F767ZI's Ethernet MAC under FreeRTOS-Plus-TCP - see
 * BddTargetBoardPlusTcp.h. The network interface is upstream's, Plus-TCP's own
 * for the STM32 MACs, over ST's HAL Ethernet driver as it stands in the pinned
 * HAL tree.
 *
 * That interface leaves the MAC's clock, pins and interrupt to the application,
 * through HAL_ETH_MspInit, which BoardEthernet.c defines for every stack. The
 * interrupt's priority is set there, when the stack first starts the interface,
 * so nothing is left to do here but name the descriptor.
 *
 * It hands the stack's own buffers to the DMA in both directions, and expects
 * them, and its descriptors, in memory the data cache does not hold: it places
 * them in sections of its own naming, which the board's linker script puts in
 * the region Board.c keeps uncached. */

#include "BddTargetBoardPlusTcp.h"

extern NetworkInterface_t* pxSTM32_FillInterfaceDescriptor(BaseType_t xEMACIndex, NetworkInterface_t* pxInterface);

NetworkInterface_t* BddTargetBoardPlusTcp_FillInterface(NetworkInterface_t* networkInterface)
{
    return pxSTM32_FillInterfaceDescriptor(0, networkInterface);
}
