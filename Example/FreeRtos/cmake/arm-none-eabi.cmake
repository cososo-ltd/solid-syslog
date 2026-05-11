# Cross toolchain for ARM Cortex-M targets — Cortex-M3 specifically for the
# QEMU mps2-an385 hello-world. Selected via the `freertos-cross` CMake preset.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# A minimum executable on bare-metal needs a linker script and our startup
# sources. CMake's compiler-id probe links a default executable; tell it to
# probe with a static library instead so the probe doesn't need our
# project-specific link line.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER   arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR           arm-none-eabi-ar)
set(CMAKE_OBJCOPY      arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP      arm-none-eabi-objdump)
set(CMAKE_RANLIB       arm-none-eabi-ranlib)

# Don't pull host program/header/library paths into the cross build.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Architecture flags applied globally so libSolidSyslog.a (compiled outside
# the executable target's PRIVATE compile_options) emits Thumb-2 instructions
# the Cortex-M3 can decode. arm-none-eabi-gcc otherwise defaults to ARM mode
# for armv7+, which links cleanly but hard-faults on the first call from
# Thumb code (slice 3b.2 hit this on SolidSyslogUdpSender_Create). Per-target
# additions in SingleTask remain idempotent on top of this.
set(CMAKE_C_FLAGS_INIT   "-mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections -fno-common")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections -fno-common")
set(CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m3 -mthumb")
