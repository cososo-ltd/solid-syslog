/* lwIP arch/cc.h for the QEMU mps2-an385 FreeRTOS+lwIP link-probe target.
 *
 * The minimum compiler-environment surface lwIP's arch.h asks an integrator
 * to supply: the diagnostic / assert hooks and the randomness source. lwIP's
 * own arch.h derives u8_t / u16_t / u32_t / s8_t / s16_t / s32_t from
 * <stdint.h> when we do not redefine them — newlib on arm-none-eabi provides
 * those, so we let lwIP's defaults stand.
 *
 * S28.07 cross-builds this image only to prove the Platform/LwipRaw tree
 * links for a Cortex-M3 FreeRTOS target; it is never run on QEMU. DIAG is a
 * no-op and ASSERT spins, matching how a deployed target would trap. LWIP_RAND
 * is a tiny self-contained xorshift so TCP ISN selection has a definition to
 * link against without dragging in newlib's rand() / a real entropy source —
 * the worked NO_SYS=0 runtime config arrives with S28.09. */
#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

// NOLINTBEGIN(bugprone-macro-parentheses) -- lwIP API requires these to be #defines
#define LWIP_PLATFORM_DIAG(x) \
    do                        \
    {                         \
        (void) 0;             \
    } while (0)
#define LWIP_PLATFORM_ASSERT(x) \
    do                          \
    {                           \
        (void) (x);             \
        for (;;)                \
        {                       \
        }                       \
    } while (0)
// NOLINTEND(bugprone-macro-parentheses)

unsigned int LwipPortRand(void);
#define LWIP_RAND() (LwipPortRand())

#endif /* LWIP_ARCH_CC_H */
