/* lwIP compiler-environment header for the consumer smoke test (S30.05).
 *
 * The minimum surface lwIP's arch.h asks an integrator to supply. lwIP derives
 * u8_t / u16_t / u32_t from <stdint.h> when they are not redefined, so newlib
 * on arm-none-eabi covers the types and only the hooks are left.
 *
 * This image is compiled, never run - LWIP_RAND is a constant rather than an
 * entropy source because nothing in the lane opens a connection. A deployed
 * target must supply a real one; see Bdd/Targets/FreeRtosLwip/arch/cc.h. */
#ifndef SOLIDSYSLOG_CONSUMER_SMOKE_ARCH_CC_H
#define SOLIDSYSLOG_CONSUMER_SMOKE_ARCH_CC_H

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

#define LWIP_RAND() (0u)

#endif /* SOLIDSYSLOG_CONSUMER_SMOKE_ARCH_CC_H */
