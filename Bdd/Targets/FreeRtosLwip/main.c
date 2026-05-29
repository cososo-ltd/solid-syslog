/* FreeRTOS + lwIP (Raw API) link-probe for QEMU mps2-an385 (Cortex-M3).
 *
 * S28.07: prove the Platform/LwipRaw adapter tree cross-builds and links for
 * a FreeRTOS/ARM target, selected via SOLIDSYSLOG_FREERTOS_NET=LWIP, with zero
 * dependency on Platform/PlusTcp or FreeRTOS-Plus-TCP. This de-risks the CMake
 * backend switch end-to-end before the heavy netif + QEMU lift (S28.09).
 *
 * It is a *link* probe, not a runtime: there is no LAN9118 netif driver, no
 * IP bring-up, and CI does not run it on QEMU. The single probe task calls
 * lwip_init() and then _Create / _Destroy on each LwipRaw adapter so the
 * linker must resolve every adapter entry point (and, transitively, the lwIP
 * core symbols they call). Starting the scheduler keeps the FreeRTOS kernel
 * symbols referenced too, so the artifact genuinely contains the kernel +
 * lwIP + LwipRaw, mirroring the FreeRTOS-Plus-TCP target's shape.
 *
 * lwipopts.h pins NO_SYS=1, so the LwipRaw default direct-call marshal is
 * correct and lwIP asks the integrator for two hooks only — sys_now() and
 * LWIP_RAND() — both provided below. */

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/init.h"
#include "lwip/sys.h"

#include "SolidSyslogLwipRawAddress.h"
#include "SolidSyslogLwipRawDatagram.h"
#include "SolidSyslogLwipRawResolver.h"
#include "SolidSyslogLwipRawTcpStream.h"

/* The probe only _Create/_Destroys adapters then idles — 4x the kernel
 * minimum is ample headroom and matches the StackType_t (word) units
 * xTaskCreate expects. */
#define PROBE_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 4U)

/* lwIP NO_SYS=1 time source — milliseconds since boot, derived from the
 * FreeRTOS tick. Drives lwIP's timeout wheel (which this probe never pumps). */
u32_t sys_now(void)
{
    return (u32_t) ((uint64_t) xTaskGetTickCount() * (uint64_t) portTICK_PERIOD_MS);
}

/* lwIP randomness source (declared by Bdd/Targets/FreeRtosLwip/arch/cc.h's
 * LWIP_RAND). A self-contained xorshift32 keeps TCP ISN selection linkable
 * without pulling in newlib rand() or a real entropy backend — the worked
 * runtime entropy story belongs to S28.09. */
unsigned int LwipPortRand(void)
{
    static uint32_t state = 0x2545F491U;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (unsigned int) state;
}

/* The bounded-connect spin in SolidSyslogLwipRawTcpStream takes a Sleep
 * callback. This probe never opens a connection, so a no-op satisfies the
 * required-field contract without behaviour. */
static void ProbeSleep(int milliseconds)
{
    (void) milliseconds;
}

static void ProbeTask(void* parameters)
{
    (void) parameters;

    lwip_init();

    struct SolidSyslogAddress* address = SolidSyslogLwipRawAddress_Create();
    struct SolidSyslogResolver* resolver = SolidSyslogLwipRawResolver_Create();
    struct SolidSyslogDatagram* datagram = SolidSyslogLwipRawDatagram_Create();

    struct SolidSyslogLwipRawTcpStreamConfig streamConfig = {
        .GetConnectTimeoutMs = NULL,
        .ConnectTimeoutContext = NULL,
        .Sleep = ProbeSleep,
    };
    struct SolidSyslogStream* stream = SolidSyslogLwipRawTcpStream_Create(&streamConfig);

    SolidSyslogLwipRawTcpStream_Destroy(stream);
    SolidSyslogLwipRawDatagram_Destroy(datagram);
    SolidSyslogLwipRawResolver_Destroy(resolver);
    SolidSyslogLwipRawAddress_Destroy(address);

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    (void) xTaskCreate(ProbeTask, "probe", PROBE_TASK_STACK_DEPTH, NULL, tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();
    for (;;)
    {
    }
    return 0;
}
