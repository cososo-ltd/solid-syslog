/* lwIP netif driver for the QEMU mps2-an385 LAN9118 (SMSC9220) Ethernet
 * controller, FreeRTOS + lwIP NO_SYS=0.
 *
 * Bridges lwIP's struct netif to the vendored Arm smsc9220_eth_drv (see
 * smsc9220/, Apache-2.0). The shape is adapted from the lwIP
 * contrib ethernetif.c skeleton (BSD, Adam Dunkels) and the FreeRTOS-Plus-TCP
 * MPS2_AN385 NetworkInterface.c (MIT) RX/IRQ pattern:
 *   - low-level output sends a (possibly chained) pbuf via
 *     smsc9220_send_by_chunks;
 *   - a dedicated RX task, unblocked by the IRQ-13 EthernetISR through a task
 *     notification, drains the RX FIFO into pbufs and hands them to lwIP via
 *     netif->input (tcpip_input under NO_SYS=0, posted to the tcpip thread). */

#include "BddTargetBoardNetif.h"

#include "BddTargetBoardConfig.h"
#include "smsc9220_emac_config.h"
#include "smsc9220_eth_drv.h"

#include "lwip/etharp.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* QEMU mps2-an385 maps the LAN9118 register block here for the Cortex-M3
 * (CORTEX_M7 would be 0xA0000000 - see the vendored SMM_MPS2.h). */
#define BOARD_NETIF_SMSC9220_BASE UINT32_C(0x40200000)

/* IRQ 13 is the LAN9118 Ethernet controller on the mps2-an385 NVIC. */
#define BOARD_NETIF_IRQ_NUMBER 13U

/* The ISR calls vTaskNotifyGiveFromISR, so its NVIC priority must be no more
 * urgent than configMAX_SYSCALL_INTERRUPT_PRIORITY. The reset default (0) is
 * more urgent and would trip configASSERT on the first FromISR call, so we set
 * it explicitly to the (least-urgent) kernel priority. */
#define BOARD_NETIF_IRQ_PRIORITY ((uint8_t) configKERNEL_INTERRUPT_PRIORITY)

#define BOARD_NETIF_RX_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 4U)
#define BOARD_NETIF_RX_TASK_PRIORITY (configMAX_PRIORITIES - 2U)

/* Wait at most this long for an RX notification before re-polling the FIFO -
 * a safety net against a missed edge, not the normal wake path. */
#define BOARD_NETIF_RX_BLOCK_MS 1500U

#define BOARD_NETIF_NAME0 'e'
#define BOARD_NETIF_NAME1 'n'

static const uint8_t BOARD_NETIF_MAC[SMSC9220_HWADDR_SIZE] = {BDD_TARGET_BOARD_MAC_OCTETS};

static const struct smsc9220_eth_dev_cfg_t BOARD_NETIF_DEV_CFG = {.base = BOARD_NETIF_SMSC9220_BASE};
static struct smsc9220_eth_dev_data_t BOARD_NETIF_DEV_DATA = {.state = 0U};
static const struct smsc9220_eth_dev_t BOARD_NETIF_DEV = {&BOARD_NETIF_DEV_CFG, &BOARD_NETIF_DEV_DATA};

/* Captured in BddTargetBoardNetif_Init so the RX task can reach netif->input. */
static struct netif* BddTargetBoardNetif_Netif = NULL;
static TaskHandle_t BddTargetBoardNetif_RxTaskHandle = NULL;

/* RX scratch: a frame is read here contiguously, then pbuf_take fans it into
 * the (possibly pool-chained) pbuf. Only the single RX task touches it. */
static char BddTargetBoardNetif_RxBuffer[SMSC9220_ETH_MAX_FRAME_SIZE];

/* Referenced by the Cortex-M3 vector table (Startup.c) at IRQ 13; the strong
 * definition below overrides Startup.c's weak Default_Handler alias. */
void EthernetISR(void);

static void BddTargetBoardNetif_LowLevelInit(struct netif* netif);
static void BddTargetBoardNetif_WaitMs(uint32_t milliseconds);
static void BddTargetBoardNetif_NvicEnableIrq(void);
static err_t BddTargetBoardNetif_LowLevelOutput(struct netif* netif, struct pbuf* p);
static void BddTargetBoardNetif_RxTask(void* parameters);
static struct pbuf* BddTargetBoardNetif_LowLevelInput(void);
static void BddTargetBoardNetif_DiscardFrame(uint32_t length);

err_t BddTargetBoardNetif_Init(struct netif* netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    BddTargetBoardNetif_Netif = netif;
    netif->name[0] = BOARD_NETIF_NAME0;
    netif->name[1] = BOARD_NETIF_NAME1;
    netif->output = etharp_output;
    netif->linkoutput = BddTargetBoardNetif_LowLevelOutput;

    BddTargetBoardNetif_LowLevelInit(netif);

    return ERR_OK;
}

static void BddTargetBoardNetif_LowLevelInit(struct netif* netif)
{
    const struct smsc9220_eth_dev_t* dev = &BOARD_NETIF_DEV;

    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    memcpy(netif->hwaddr, BOARD_NETIF_MAC, SMSC9220_HWADDR_SIZE);
    netif->mtu = SMSC9220_ETH_MTU_SIZE;
    netif->flags = (u8_t) (NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP);

    /* The ISR notifies this task, so it must exist before the IRQ is enabled. */
    (void) xTaskCreate(
        BddTargetBoardNetif_RxTask,
        "ethrx",
        BOARD_NETIF_RX_TASK_STACK_DEPTH,
        NULL,
        BOARD_NETIF_RX_TASK_PRIORITY,
        &BddTargetBoardNetif_RxTaskHandle
    );

    (void) smsc9220_init(dev, BddTargetBoardNetif_WaitMs);

    smsc9220_disable_all_interrupts(dev);
    smsc9220_clear_all_interrupts(dev);
    (void) smsc9220_set_fifo_level_irq(dev, SMSC9220_FIFO_LEVEL_IRQ_RX_STATUS_POS, SMSC9220_FIFO_LEVEL_IRQ_LEVEL_MIN);
    (void) smsc9220_set_fifo_level_irq(dev, SMSC9220_FIFO_LEVEL_IRQ_TX_STATUS_POS, SMSC9220_FIFO_LEVEL_IRQ_LEVEL_MIN);
    (void) smsc9220_set_fifo_level_irq(dev, SMSC9220_FIFO_LEVEL_IRQ_TX_DATA_POS, SMSC9220_FIFO_LEVEL_IRQ_LEVEL_MAX);

    uint32_t macLow = 0U;
    uint32_t macHigh = 0U;
    memcpy(&macLow, BOARD_NETIF_MAC, 4U);
    memcpy(&macHigh, &BOARD_NETIF_MAC[4], 2U);
    (void) smsc9220_mac_regwrite(dev, SMSC9220_MAC_REG_OFFSET_ADDRL, macLow);
    (void) smsc9220_mac_regwrite(dev, SMSC9220_MAC_REG_OFFSET_ADDRH, macHigh);

    smsc9220_enable_interrupt(dev, SMSC9220_INTERRUPT_RX_STATUS_FIFO_LEVEL);

    BddTargetBoardNetif_NvicEnableIrq();
}

static void BddTargetBoardNetif_WaitMs(uint32_t milliseconds)
{
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
}

static void BddTargetBoardNetif_NvicEnableIrq(void)
{
    // NOLINTNEXTLINE(performance-no-int-to-ptr) -- Cortex-M3 NVIC IPR byte for IRQ 13.
    volatile uint8_t* ipr = (volatile uint8_t*) (UINT32_C(0xE000E400) + BOARD_NETIF_IRQ_NUMBER);
    *ipr = BOARD_NETIF_IRQ_PRIORITY;

    // NOLINTNEXTLINE(performance-no-int-to-ptr) -- Cortex-M3 NVIC ISER (set-enable) register.
    volatile uint32_t* iser = (volatile uint32_t*) UINT32_C(0xE000E100);
    *iser = (uint32_t) (1UL << (BOARD_NETIF_IRQ_NUMBER & 0x1FU));
}

static err_t BddTargetBoardNetif_LowLevelOutput(struct netif* netif, struct pbuf* p)
{
    (void) netif;
    const struct smsc9220_eth_dev_t* dev = &BOARD_NETIF_DEV;
    enum smsc9220_error_t error = SMSC9220_ERROR_NONE;

    for (struct pbuf* q = p; (q != NULL) && (error == SMSC9220_ERROR_NONE); q = q->next)
    {
        error = smsc9220_send_by_chunks(dev, p->tot_len, (q == p), (const char*) q->payload, q->len);
    }

    err_t result = ERR_IF;
    if (error == SMSC9220_ERROR_NONE)
    {
        result = ERR_OK;
        LINK_STATS_INC(link.xmit);
    }
    else
    {
        LINK_STATS_INC(link.err);
    }
    return result;
}

static void BddTargetBoardNetif_RxTask(void* parameters)
{
    (void) parameters;
    const struct smsc9220_eth_dev_t* dev = &BOARD_NETIF_DEV;

    for (;;)
    {
        (void) ulTaskNotifyTake(pdFALSE, pdMS_TO_TICKS(BOARD_NETIF_RX_BLOCK_MS));

        struct pbuf* p = BddTargetBoardNetif_LowLevelInput();
        while (p != NULL)
        {
            if (BddTargetBoardNetif_Netif->input(p, BddTargetBoardNetif_Netif) != ERR_OK)
            {
                pbuf_free(p);
            }
            p = BddTargetBoardNetif_LowLevelInput();
        }

        /* The ISR disabled the RX interrupt; re-arm it now the FIFO is drained. */
        smsc9220_enable_interrupt(dev, SMSC9220_INTERRUPT_RX_STATUS_FIFO_LEVEL);
    }
}

static struct pbuf* BddTargetBoardNetif_LowLevelInput(void)
{
    const struct smsc9220_eth_dev_t* dev = &BOARD_NETIF_DEV;
    struct pbuf* p = NULL;

    uint32_t length = smsc9220_peek_next_packet_size(dev);
    if ((length != 0U) && (length <= sizeof(BddTargetBoardNetif_RxBuffer)))
    {
        uint32_t received = smsc9220_receive_by_chunks(dev, BddTargetBoardNetif_RxBuffer, length);
        p = pbuf_alloc(PBUF_RAW, (u16_t) received, PBUF_POOL);
        if (p != NULL)
        {
            (void) pbuf_take(p, BddTargetBoardNetif_RxBuffer, (u16_t) received);
            LINK_STATS_INC(link.recv);
        }
        else
        {
            LINK_STATS_INC(link.memerr);
            LINK_STATS_INC(link.drop);
        }
    }
    else if (length > sizeof(BddTargetBoardNetif_RxBuffer))
    {
        /* An oversize/corrupt frame must still be pulled from the RX FIFO or it
         * is peeked again forever and wedges all subsequent receive. */
        BddTargetBoardNetif_DiscardFrame(length);
        LINK_STATS_INC(link.lenerr);
        LINK_STATS_INC(link.drop);
    }
    return p;
}

/* The RX data FIFO must be drained by the frame's full word-aligned footprint
 * or the read pointer desyncs. The vendored driver only reads a whole packet
 * into a caller buffer, so drain a too-large frame in buffer-sized,
 * word-aligned chunks (the final, possibly shorter, chunk carries the
 * sub-word remainder) and discard each. */
static void BddTargetBoardNetif_DiscardFrame(uint32_t length)
{
    const struct smsc9220_eth_dev_t* dev = &BOARD_NETIF_DEV;
    const uint32_t chunkCap = sizeof(BddTargetBoardNetif_RxBuffer) & ~3U;

    while (length > 0U)
    {
        uint32_t chunk = (length > chunkCap) ? chunkCap : length;
        (void) smsc9220_receive_by_chunks(dev, BddTargetBoardNetif_RxBuffer, chunk);
        length -= chunk;
    }
}

void EthernetISR(void)
{
    const struct smsc9220_eth_dev_t* dev = &BOARD_NETIF_DEV;
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    const uint32_t rxFifoStatusBit = 1UL << SMSC9220_INTERRUPT_RX_STATUS_FIFO_LEVEL;

    /* Defined non-static in the vendored smsc9220_eth_drv.c. */
    extern uint32_t get_irq_status(const struct smsc9220_eth_dev_t* dev);

    if ((get_irq_status(dev) & rxFifoStatusBit) != 0U)
    {
        vTaskNotifyGiveFromISR(BddTargetBoardNetif_RxTaskHandle, &higherPriorityTaskWoken);
        smsc9220_clear_interrupt(dev, SMSC9220_INTERRUPT_RX_STATUS_FIFO_LEVEL);
        /* Re-enabled by the RX task once it has drained the FIFO. */
        smsc9220_disable_interrupt(dev, SMSC9220_INTERRUPT_RX_STATUS_FIFO_LEVEL);
    }
    smsc9220_clear_all_interrupts(dev);

    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}
