/* Host-side test shim for lwIP's arch/cc.h. Minimum surface needed to
 * compile lwIP headers under host GCC / Clang builds: LWIP_PLATFORM_DIAG
 * / _ASSERT as no-ops so unit tests don't print or abort on traffic-
 * pattern messages. lwIP's own arch.h supplies u8_t / u16_t / u32_t /
 * s8_t / s16_t / s32_t via stdint when we don't redefine them — let it. */
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
    } while (0)
// NOLINTEND(bugprone-macro-parentheses)

#endif /* LWIP_ARCH_CC_H */
