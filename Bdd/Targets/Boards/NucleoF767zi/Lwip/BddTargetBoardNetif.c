/* lwIP netif for the NUCLEO-F767ZI's Ethernet MAC and its on-board LAN8742A
 * PHY, over RMII - see BddTargetBoardNetif.h.
 *
 * The MAC and its DMA are driven through ST's HAL Ethernet driver, unmodified.
 * This file is the glue either side of it:
 *   - transmit copies the pbuf chain into one buffer and waits for the DMA to
 *     send it;
 *   - one task receives and watches the link. The Ethernet interrupt wakes it
 *     for a frame, which it copies into a pbuf for netif->input. Between
 *     frames it reads the link from the PHY, starts or stops the MAC to
 *     match, tells lwIP and reports the change on the console.
 *
 * The MAC's clock, pins and interrupt, and the PHY's restart before the MAC's
 * reset, are in BoardEthernet.c, shared by every stack.
 *
 * Every frame is copied, in both directions, between lwIP's memory and
 * buffers the DMA owns. Those buffers and the DMA's descriptors are static,
 * in the memory the board support keeps out of the data cache, so no cache
 * maintenance is needed here.
 *
 * Cross build only, so not host-tested beyond the link decoding in
 * EthernetPhy: the target checklist in the board's README.md covers it on
 * the board. */

#include "BddTargetBoardNetif.h"

#include "BddTargetBoardConfig.h"
#include "BoardEthernet.h"
#include "EthernetPhy.h"

#include "lwip/etharp.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/tcpip.h"

#include "FreeRTOS.h"
#include "task.h"

#include "stm32f7xx_hal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum
{
    BOARD_NETIF_MTU = 1500,

    /* One buffer holds any frame the MAC accepts, so a frame never spans two
     * descriptors. A multiple of the cache line. */
    BOARD_NETIF_BUFFER_BYTES = 1536,
    BOARD_NETIF_BUFFER_ALIGNMENT = 32,

    /* The HAL takes a buffer for each descriptor it rebuilds before the frame
     * just read has released its own, so the pool is larger than the ring. */
    BOARD_NETIF_RX_BUFFER_COUNT = 2 * ETH_RX_DESC_CNT,

    BOARD_NETIF_TASK_STACK_WORDS = configMINIMAL_STACK_SIZE * 8,
    BOARD_NETIF_TASK_PRIORITY = configMAX_PRIORITIES - 2,

    /* How long the task waits for a frame before it checks anyway, and how
     * often it reads the link from the PHY. */
    BOARD_NETIF_RX_WAIT_MILLISECONDS = 100,
    BOARD_NETIF_LINK_POLL_MILLISECONDS = 250,

    BOARD_NETIF_TRANSMIT_TIMEOUT_MILLISECONDS = 100,

    BOARD_NETIF_REPORT_BYTES = 64
};

/* In the linker script's uncached region, aligned as the DMA requires. */
#define BOARD_NETIF_DMA_MEMORY __attribute__((section(".ethernet_dma"), aligned(BOARD_NETIF_BUFFER_ALIGNMENT)))

static uint8_t BddTargetBoardNetif_Mac[ETHARP_HWADDR_LEN] = {BDD_TARGET_BOARD_MAC_OCTETS};

static ETH_HandleTypeDef BddTargetBoardNetif_Ethernet;
static ETH_DMADescTypeDef BddTargetBoardNetif_RxDescriptors[ETH_RX_DESC_CNT] BOARD_NETIF_DMA_MEMORY;
static ETH_DMADescTypeDef BddTargetBoardNetif_TxDescriptors[ETH_TX_DESC_CNT] BOARD_NETIF_DMA_MEMORY;

static uint8_t BddTargetBoardNetif_TxBuffer[BOARD_NETIF_BUFFER_BYTES] BOARD_NETIF_DMA_MEMORY;
static uint8_t BddTargetBoardNetif_RxBuffers[BOARD_NETIF_RX_BUFFER_COUNT]
                                            [BOARD_NETIF_BUFFER_BYTES] BOARD_NETIF_DMA_MEMORY;
static bool BddTargetBoardNetif_RxBufferInUse[BOARD_NETIF_RX_BUFFER_COUNT];

/* The frame HAL_ETH_ReadData is returning, as its link callback recorded it:
 * its buffers and its length. Only the task touches these. */
static uint8_t* BddTargetBoardNetif_FrameBuffers[ETH_RX_DESC_CNT];
static uint32_t BddTargetBoardNetif_FrameBufferCount;
static uint32_t BddTargetBoardNetif_FrameLength;

/* Captured in BddTargetBoardNetif_Init so the task can reach netif->input. */
static struct netif* BddTargetBoardNetif_Netif = NULL;
static TaskHandle_t BddTargetBoardNetif_TaskHandle = NULL;
static enum EthernetPhyLink BddTargetBoardNetif_Link = ETHERNET_PHY_LINK_DOWN;

/* The name in ST's vector table. */
void ETH_IRQHandler(void);

static err_t BddTargetBoardNetif_LowLevelOutput(struct netif* netif, struct pbuf* p);
static void BddTargetBoardNetif_Task(void* parameters);
static void BddTargetBoardNetif_ReceiveFrames(void);
static void BddTargetBoardNetif_DeliverFrame(void);
static void BddTargetBoardNetif_ReleaseFrame(void);
static void BddTargetBoardNetif_FollowLink(void);
static void BddTargetBoardNetif_ApplyLink(enum EthernetPhyLink link);
static void BddTargetBoardNetif_StartMac(enum EthernetPhyLink link);
static void BddTargetBoardNetif_Report(const char* what, const char* detail);

err_t BddTargetBoardNetif_Init(struct netif* netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    BddTargetBoardNetif_Netif = netif;
    netif->name[0] = 'e';
    netif->name[1] = 'n';
    netif->output = etharp_output;
    netif->linkoutput = BddTargetBoardNetif_LowLevelOutput;
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    memcpy(netif->hwaddr, BddTargetBoardNetif_Mac, sizeof(BddTargetBoardNetif_Mac));
    netif->mtu = BOARD_NETIF_MTU;
    /* No NETIF_FLAG_LINK_UP: the task raises the link when the PHY has one. */
    netif->flags = (u8_t) (NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP);

    BddTargetBoardNetif_Ethernet.Instance = ETH;
    BddTargetBoardNetif_Ethernet.Init.MACAddr = BddTargetBoardNetif_Mac;
    BddTargetBoardNetif_Ethernet.Init.MediaInterface = HAL_ETH_RMII_MODE;
    BddTargetBoardNetif_Ethernet.Init.RxDesc = BddTargetBoardNetif_RxDescriptors;
    BddTargetBoardNetif_Ethernet.Init.TxDesc = BddTargetBoardNetif_TxDescriptors;
    BddTargetBoardNetif_Ethernet.Init.RxBuffLen = BOARD_NETIF_BUFFER_BYTES;

    /* The MAC's reset completes only with the PHY's 50 MHz reference clock
     * running, so a failure here is a PHY that is not clocking. The interface
     * stays, with no link. */
    if (HAL_ETH_Init(&BddTargetBoardNetif_Ethernet) == HAL_OK)
    {
        /* The HAL leaves the management clock divider at its reset value,
         * which at this core clock runs the bus faster than a PHY answers. */
        HAL_ETH_SetMDIOClockRange(&BddTargetBoardNetif_Ethernet);
        (void) xTaskCreate(
            BddTargetBoardNetif_Task,
            "ethernet",
            (configSTACK_DEPTH_TYPE) BOARD_NETIF_TASK_STACK_WORDS,
            NULL,
            (UBaseType_t) BOARD_NETIF_TASK_PRIORITY,
            &BddTargetBoardNetif_TaskHandle
        );
    }
    else
    {
        BddTargetBoardNetif_Report("ethernet", "the MAC did not start");
    }
    return ERR_OK;
}

/* lwIP calls this holding its core lock, which the task also holds to start
 * and stop the MAC, so a frame is never given to a MAC that is changing. */
static err_t BddTargetBoardNetif_LowLevelOutput(struct netif* netif, struct pbuf* p)
{
    (void) netif;
    err_t result = ERR_IF;
    if (p->tot_len <= sizeof(BddTargetBoardNetif_TxBuffer))
    {
        ETH_BufferTypeDef buffer = {0};
        ETH_TxPacketConfigTypeDef packet = {0};
        buffer.buffer = BddTargetBoardNetif_TxBuffer;
        buffer.len = pbuf_copy_partial(p, BddTargetBoardNetif_TxBuffer, p->tot_len, 0U);
        /* lwIP has already computed every checksum; the MAC adds the padding
         * and the CRC. */
        packet.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
        packet.ChecksumCtrl = ETH_CHECKSUM_DISABLE;
        packet.CRCPadCtrl = ETH_CRC_PAD_INSERT;
        packet.Length = buffer.len;
        packet.TxBuffer = &buffer;
        /* The polled transmit returns with the frame sent, so there is
         * nothing to release afterwards. */
        HAL_StatusTypeDef sent = HAL_ETH_Transmit(
            &BddTargetBoardNetif_Ethernet,
            &packet,
            (uint32_t) BOARD_NETIF_TRANSMIT_TIMEOUT_MILLISECONDS
        );
        if (sent == HAL_OK)
        {
            result = ERR_OK;
        }
    }
    if (result == ERR_OK)
    {
        LINK_STATS_INC(link.xmit);
    }
    else
    {
        LINK_STATS_INC(link.err);
    }
    return result;
}

static void BddTargetBoardNetif_Task(void* parameters)
{
    (void) parameters;
    TickType_t lastLinkPoll = xTaskGetTickCount();

    BddTargetBoardNetif_Report("link", EthernetPhy_LinkText(BddTargetBoardNetif_Link));
    for (;;)
    {
        (void) ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BOARD_NETIF_RX_WAIT_MILLISECONDS));
        BddTargetBoardNetif_ReceiveFrames();
        if ((xTaskGetTickCount() - lastLinkPoll) >= pdMS_TO_TICKS(BOARD_NETIF_LINK_POLL_MILLISECONDS))
        {
            lastLinkPoll = xTaskGetTickCount();
            BddTargetBoardNetif_FollowLink();
        }
    }
}

/* HAL_ETH_ReadData also rebuilds the descriptors of the frames taken so far,
 * so it is called until it has nothing, not only while frames are expected.
 * While the MAC is stopped it returns HAL_ERROR, which is harmless. */
static void BddTargetBoardNetif_ReceiveFrames(void)
{
    void* frame = NULL;
    while (HAL_ETH_ReadData(&BddTargetBoardNetif_Ethernet, &frame) == HAL_OK)
    {
        BddTargetBoardNetif_DeliverFrame();
        BddTargetBoardNetif_ReleaseFrame();
    }
}

/* The HAL asks for a buffer for each receive descriptor it builds. Returning
 * NULL makes it try again on the next HAL_ETH_ReadData. The NULL has to be
 * written explicitly, because the HAL passes in whatever the previous
 * descriptor was given. */
void HAL_ETH_RxAllocateCallback(uint8_t** buff) // NOLINT(readability-identifier-naming) -- ST's name
{
    *buff = NULL;
    for (uint32_t i = 0U; i < (uint32_t) BOARD_NETIF_RX_BUFFER_COUNT; i++)
    {
        if (!BddTargetBoardNetif_RxBufferInUse[i])
        {
            BddTargetBoardNetif_RxBufferInUse[i] = true;
            *buff = BddTargetBoardNetif_RxBuffers[i];
            break;
        }
    }
}

/* The HAL passes each buffer of the frame it is reading, in order. *pStart is
 * what HAL_ETH_ReadData returns, and only has to be non-NULL: the frame is
 * recorded here. */
// NOLINTNEXTLINE(readability-identifier-naming) -- ST's name
void HAL_ETH_RxLinkCallback(void** pStart, void** pEnd, uint8_t* buff, uint16_t Length)
{
    if (*pStart == NULL)
    {
        *pStart = buff;
        BddTargetBoardNetif_FrameBufferCount = 0U;
        BddTargetBoardNetif_FrameLength = 0U;
    }
    *pEnd = buff;
    if (BddTargetBoardNetif_FrameBufferCount < (uint32_t) ETH_RX_DESC_CNT)
    {
        BddTargetBoardNetif_FrameBuffers[BddTargetBoardNetif_FrameBufferCount] = buff;
        BddTargetBoardNetif_FrameBufferCount++;
    }
    BddTargetBoardNetif_FrameLength += Length;
}

/* A frame in more than one buffer is longer than any the interface carries,
 * and is dropped. */
static void BddTargetBoardNetif_DeliverFrame(void)
{
    bool fitsOneBuffer = (BddTargetBoardNetif_FrameBufferCount == 1U) &&
                         (BddTargetBoardNetif_FrameLength <= (uint32_t) BOARD_NETIF_BUFFER_BYTES);
    struct pbuf* p = NULL;
    if (fitsOneBuffer)
    {
        p = pbuf_alloc(PBUF_RAW, (u16_t) BddTargetBoardNetif_FrameLength, PBUF_POOL);
    }
    if (p != NULL)
    {
        (void) pbuf_take(p, BddTargetBoardNetif_FrameBuffers[0], (u16_t) BddTargetBoardNetif_FrameLength);
        LINK_STATS_INC(link.recv);
        if (BddTargetBoardNetif_Netif->input(p, BddTargetBoardNetif_Netif) != ERR_OK)
        {
            pbuf_free(p);
        }
    }
    else
    {
        LINK_STATS_INC(link.drop);
    }
}

static void BddTargetBoardNetif_ReleaseFrame(void)
{
    for (uint32_t i = 0U; i < BddTargetBoardNetif_FrameBufferCount; i++)
    {
        size_t offset = (size_t) (BddTargetBoardNetif_FrameBuffers[i] - BddTargetBoardNetif_RxBuffers[0]);
        BddTargetBoardNetif_RxBufferInUse[offset / (size_t) BOARD_NETIF_BUFFER_BYTES] = false;
    }
    BddTargetBoardNetif_FrameBufferCount = 0U;
    BddTargetBoardNetif_FrameLength = 0U;
}

static void BddTargetBoardNetif_FollowLink(void)
{
    enum EthernetPhyLink link = EthernetPhy_ReadLink(BoardEthernet_ReadPhy, &BddTargetBoardNetif_Ethernet);
    if (link != BddTargetBoardNetif_Link)
    {
        BddTargetBoardNetif_ApplyLink(link);
        BddTargetBoardNetif_Link = link;
        BddTargetBoardNetif_Report("link", EthernetPhy_LinkText(link));
    }
}

/* A change of speed or duplex with no link drop also goes through a stop: the
 * MAC takes its configuration only while stopped. */
static void BddTargetBoardNetif_ApplyLink(enum EthernetPhyLink link)
{
    LOCK_TCPIP_CORE();
    if (EthernetPhy_IsUp(BddTargetBoardNetif_Link))
    {
        (void) HAL_ETH_Stop_IT(&BddTargetBoardNetif_Ethernet);
        netif_set_link_down(BddTargetBoardNetif_Netif);
    }
    if (EthernetPhy_IsUp(link))
    {
        BddTargetBoardNetif_StartMac(link);
        netif_set_link_up(BddTargetBoardNetif_Netif);
    }
    UNLOCK_TCPIP_CORE();
}

static void BddTargetBoardNetif_StartMac(enum EthernetPhyLink link)
{
    ETH_MACConfigTypeDef mac = {0};
    (void) HAL_ETH_GetMACConfig(&BddTargetBoardNetif_Ethernet, &mac);
    mac.Speed = EthernetPhy_Is100Megabit(link) ? ETH_SPEED_100M : ETH_SPEED_10M;
    mac.DuplexMode = EthernetPhy_IsFullDuplex(link) ? ETH_FULLDUPLEX_MODE : ETH_HALFDUPLEX_MODE;
    (void) HAL_ETH_SetMACConfig(&BddTargetBoardNetif_Ethernet, &mac);
    (void) HAL_ETH_Start_IT(&BddTargetBoardNetif_Ethernet);
}

/* Written with one write() and not stdio, which belongs to the target's
 * interactive thread; the console lock in _write keeps the line whole. */
static void BddTargetBoardNetif_Report(const char* what, const char* detail)
{
    char line[BOARD_NETIF_REPORT_BYTES];
    int formatted = snprintf(line, sizeof(line), "[board] %s %s\n", what, detail);
    if ((formatted > 0) && ((size_t) formatted < sizeof(line)))
    {
        (void) write(STDOUT_FILENO, line, (size_t) formatted);
    }
}

void ETH_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&BddTargetBoardNetif_Ethernet);
}

/* From the interrupt, when a frame has arrived. */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef* heth) // NOLINT(readability-identifier-naming) -- ST's name
{
    (void) heth;
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(BddTargetBoardNetif_TaskHandle, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

/* From the interrupt on an error, such as a frame that found no descriptor
 * free. Waking the task makes it read what is waiting, which rebuilds the
 * descriptors. */
void HAL_ETH_ErrorCallback(ETH_HandleTypeDef* heth) // NOLINT(readability-identifier-naming) -- ST's name
{
    HAL_ETH_RxCpltCallback(heth);
}
