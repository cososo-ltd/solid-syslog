/* lwIP arch/cc.h for the cross BDD targets that run lwIP.
 *
 * The minimum compiler-environment surface lwIP's arch.h asks an integrator
 * to supply: the diagnostic / assert hooks and the randomness source. lwIP's
 * own arch.h derives u8_t / u16_t / u32_t / s8_t / s16_t / s32_t from
 * <stdint.h> when we do not redefine them - newlib on arm-none-eabi provides
 * those, so we let lwIP's defaults stand.
 *
 * DIAG is a no-op. ASSERT says on the console what lwIP found, and stops the
 * target. LWIP_RAND is a tiny self-contained xorshift, defined in each target's
 * main.c, so TCP ISN selection has a definition to link against without
 * dragging in newlib's rand() / a real entropy source. */
#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#include "../../BddTargetBoard.h"

// NOLINTBEGIN(bugprone-macro-parentheses) -- lwIP API requires these to be #defines
#define LWIP_PLATFORM_DIAG(x) \
    do                        \
    {                         \
        (void) 0;             \
    } while (0)
#define LWIP_PLATFORM_ASSERT(x)                      \
    do                                               \
    {                                                \
        BddTargetBoard_Fault("lwIP assertion", (x)); \
    } while (0)
// NOLINTEND(bugprone-macro-parentheses)

unsigned int LwipPortRand(void);
#define LWIP_RAND() (LwipPortRand())

#endif /* LWIP_ARCH_CC_H */
