/* lwIP netif driver for the QEMU mps2-an385 LAN9118 (SMSC9220) Ethernet
 * controller, FreeRTOS + lwIP NO_SYS=0.
 *
 * Bridges lwIP's struct netif to the vendored Arm smsc9220_eth_drv (see
 * netif/smsc9220/, Apache-2.0). The shape is adapted from the lwIP
 * contrib ethernetif.c skeleton (BSD, Adam Dunkels) and the FreeRTOS-Plus-TCP
 * MPS2_AN385 NetworkInterface.c (MIT) RX/IRQ pattern:
 *   - low-level output sends a (possibly chained) pbuf via
 *     smsc9220_send_by_chunks;
 *   - a dedicated RX task, unblocked by the IRQ-13 EthernetISR through a task
 *     notification, drains the RX FIFO into pbufs and hands them to lwIP via
 *     netif->input (tcpip_input under NO_SYS=0, posted to the tcpip thread). */

#include "EthernetIf.h"

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
 * (CORTEX_M7 would be 0xA0000000 — see the vendored SMM_MPS2.h). */
#define ETHERNETIF_SMSC9220_BASE UINT32_C(0x40200000)

/* IRQ 13 is the LAN9118 Ethernet controller on the mps2-an385 NVIC. */
#define ETHERNETIF_IRQ_NUMBER 13U

/* The ISR calls vTaskNotifyGiveFromISR, so its NVIC priority must be no more
 * urgent than configMAX_SYSCALL_INTERRUPT_PRIORITY. The reset default (0) is
 * more urgent and would trip configASSERT on the first FromISR call, so we set
 * it explicitly to the (least-urgent) kernel priority. */
#define ETHERNETIF_IRQ_PRIORITY ((uint8_t) configKERNEL_INTERRUPT_PRIORITY)

#define ETHERNETIF_RX_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 4U)
#define ETHERNETIF_RX_TASK_PRIORITY (configMAX_PRIORITIES - 2U)

/* Wait at most this long for an RX notification before re-polling the FIFO —
 * a safety net against a missed edge, not the normal wake path. */
#define ETHERNETIF_RX_BLOCK_MS 1500U

#define ETHERNETIF_NAME0 'e'
#define ETHERNETIF_NAME1 'n'

/* Locally administered unicast MAC, matching the +TCP reference target. */
static const uint8_t ETHERNETIF_MAC[SMSC9220_HWADDR_SIZE] = {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U};

static const struct smsc9220_eth_dev_cfg_t ETHERNETIF_DEV_CFG = {.base = ETHERNETIF_SMSC9220_BASE};
static struct smsc9220_eth_dev_data_t ETHERNETIF_DEV_DATA = {.state = 0U};
static const struct smsc9220_eth_dev_t ETHERNETIF_DEV = {&ETHERNETIF_DEV_CFG, &ETHERNETIF_DEV_DATA};

/* Captured in EthernetIf_Init so the RX task can reach netif->input. */
static struct netif* EthernetIf_Netif = NULL;
static TaskHandle_t EthernetIf_RxTaskHandle = NULL;

/* RX scratch: a frame is read here contiguously, then pbuf_take fans it into
 * the (possibly pool-chained) pbuf. Only the single RX task touches it. */
static char EthernetIf_RxBuffer[SMSC9220_ETH_MAX_FRAME_SIZE];

/* Referenced by the Cortex-M3 vector table (Startup.c) at IRQ 13; the strong
 * definition below overrides Startup.c's weak Default_Handler alias. */
void EthernetISR(void);

static void EthernetIf_LowLevelInit(struct netif* netif);
static void EthernetIf_WaitMs(uint32_t milliseconds);
static void EthernetIf_NvicEnableIrq(void);
static err_t EthernetIf_LowLevelOutput(struct netif* netif, struct pbuf* p);
static void EthernetIf_RxTask(void* parameters);
static struct pbuf* EthernetIf_LowLevelInput(void);

err_t EthernetIf_Init(struct netif* netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    EthernetIf_Netif = netif;
    netif->name[0] = ETHERNETIF_NAME0;
    netif->name[1] = ETHERNETIF_NAME1;
    netif->output = etharp_output;
    netif->linkoutput = EthernetIf_LowLevelOutput;

    EthernetIf_LowLevelInit(netif);

    return ERR_OK;
}

static void EthernetIf_LowLevelInit(struct netif* netif)
{
    const struct smsc9220_eth_dev_t* dev = &ETHERNETIF_DEV;

    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    memcpy(netif->hwaddr, ETHERNETIF_MAC, SMSC9220_HWADDR_SIZE);
    netif->mtu = SMSC9220_ETH_MTU_SIZE;
    netif->flags = (u8_t) (NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP);

    /* The ISR notifies this task, so it must exist before the IRQ is enabled. */
    (void) xTaskCreate(
        EthernetIf_RxTask,
        "ethrx",
        ETHERNETIF_RX_TASK_STACK_DEPTH,
        NULL,
        ETHERNETIF_RX_TASK_PRIORITY,
        &EthernetIf_RxTaskHandle
    );

    (void) smsc9220_init(dev, EthernetIf_WaitMs);

    smsc9220_disable_all_interrupts(dev);
    smsc9220_clear_all_interrupts(dev);
    (void) smsc9220_set_fifo_level_irq(dev, SMSC9220_FIFO_LEVEL_IRQ_RX_STATUS_POS, SMSC9220_FIFO_LEVEL_IRQ_LEVEL_MIN);
    (void) smsc9220_set_fifo_level_irq(dev, SMSC9220_FIFO_LEVEL_IRQ_TX_STATUS_POS, SMSC9220_FIFO_LEVEL_IRQ_LEVEL_MIN);
    (void) smsc9220_set_fifo_level_irq(dev, SMSC9220_FIFO_LEVEL_IRQ_TX_DATA_POS, SMSC9220_FIFO_LEVEL_IRQ_LEVEL_MAX);

    uint32_t macLow = 0U;
    uint32_t macHigh = 0U;
    memcpy(&macLow, ETHERNETIF_MAC, 4U);
    memcpy(&macHigh, &ETHERNETIF_MAC[4], 2U);
    (void) smsc9220_mac_regwrite(dev, SMSC9220_MAC_REG_OFFSET_ADDRL, macLow);
    (void) smsc9220_mac_regwrite(dev, SMSC9220_MAC_REG_OFFSET_ADDRH, macHigh);

    smsc9220_enable_interrupt(dev, SMSC9220_INTERRUPT_RX_STATUS_FIFO_LEVEL);

    EthernetIf_NvicEnableIrq();
}

static void EthernetIf_WaitMs(uint32_t milliseconds)
{
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
}

static void EthernetIf_NvicEnableIrq(void)
{
    // NOLINTNEXTLINE(performance-no-int-to-ptr) -- Cortex-M3 NVIC IPR byte for IRQ 13.
    volatile uint8_t* ipr = (volatile uint8_t*) (UINT32_C(0xE000E400) + ETHERNETIF_IRQ_NUMBER);
    *ipr = ETHERNETIF_IRQ_PRIORITY;

    // NOLINTNEXTLINE(performance-no-int-to-ptr) -- Cortex-M3 NVIC ISER (set-enable) register.
    volatile uint32_t* iser = (volatile uint32_t*) UINT32_C(0xE000E100);
    *iser = (uint32_t) (1UL << (ETHERNETIF_IRQ_NUMBER & 0x1FU));
}

static err_t EthernetIf_LowLevelOutput(struct netif* netif, struct pbuf* p)
{
    (void) netif;
    const struct smsc9220_eth_dev_t* dev = &ETHERNETIF_DEV;
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

static void EthernetIf_RxTask(void* parameters)
{
    (void) parameters;
    const struct smsc9220_eth_dev_t* dev = &ETHERNETIF_DEV;

    for (;;)
    {
        (void) ulTaskNotifyTake(pdFALSE, pdMS_TO_TICKS(ETHERNETIF_RX_BLOCK_MS));

        struct pbuf* p = EthernetIf_LowLevelInput();
        while (p != NULL)
        {
            if (EthernetIf_Netif->input(p, EthernetIf_Netif) != ERR_OK)
            {
                pbuf_free(p);
            }
            p = EthernetIf_LowLevelInput();
        }

        /* The ISR disabled the RX interrupt; re-arm it now the FIFO is drained. */
        smsc9220_enable_interrupt(dev, SMSC9220_INTERRUPT_RX_STATUS_FIFO_LEVEL);
    }
}

static struct pbuf* EthernetIf_LowLevelInput(void)
{
    const struct smsc9220_eth_dev_t* dev = &ETHERNETIF_DEV;
    struct pbuf* p = NULL;

    uint32_t length = smsc9220_peek_next_packet_size(dev);
    if ((length != 0U) && (length <= sizeof(EthernetIf_RxBuffer)))
    {
        uint32_t received = smsc9220_receive_by_chunks(dev, EthernetIf_RxBuffer, length);
        p = pbuf_alloc(PBUF_RAW, (u16_t) received, PBUF_POOL);
        if (p != NULL)
        {
            (void) pbuf_take(p, EthernetIf_RxBuffer, (u16_t) received);
            LINK_STATS_INC(link.recv);
        }
        else
        {
            LINK_STATS_INC(link.memerr);
            LINK_STATS_INC(link.drop);
        }
    }
    return p;
}

void EthernetISR(void)
{
    const struct smsc9220_eth_dev_t* dev = &ETHERNETIF_DEV;
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    const uint32_t rxFifoStatusBit = 1UL << SMSC9220_INTERRUPT_RX_STATUS_FIFO_LEVEL;

    /* Defined non-static in the vendored smsc9220_eth_drv.c. */
    extern uint32_t get_irq_status(const struct smsc9220_eth_dev_t* dev);

    if ((get_irq_status(dev) & rxFifoStatusBit) != 0U)
    {
        vTaskNotifyGiveFromISR(EthernetIf_RxTaskHandle, &higherPriorityTaskWoken);
        smsc9220_clear_interrupt(dev, SMSC9220_INTERRUPT_RX_STATUS_FIFO_LEVEL);
        /* Re-enabled by the RX task once it has drained the FIFO. */
        smsc9220_disable_interrupt(dev, SMSC9220_INTERRUPT_RX_STATUS_FIFO_LEVEL);
    }
    smsc9220_clear_all_interrupts(dev);

    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}
