# Cross toolchain for ARM Cortex-M targets — Cortex-M3 specifically for the
# QEMU mps2-an385 hello-world. Selected via the `freertos-cross` CMake preset.

include("${CMAKE_CURRENT_LIST_DIR}/../../Common/cmake/arm-none-eabi-common.cmake")

# Architecture flags applied globally so libSolidSyslog.a (compiled outside
# the executable target's PRIVATE compile_options) emits Thumb-2 instructions
# the Cortex-M3 can decode. arm-none-eabi-gcc otherwise defaults to ARM mode
# for armv7+, which links cleanly but hard-faults on the first call from
# Thumb code (slice 3b.2 hit this on SolidSyslogUdpSender_Create). Per-target
# additions in SingleTask remain idempotent on top of this.
set(CMAKE_C_FLAGS_INIT   "-mcpu=cortex-m3 -mthumb ${SOLIDSYSLOG_ARM_NONE_EABI_SECTION_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m3 -mthumb ${SOLIDSYSLOG_ARM_NONE_EABI_SECTION_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m3 -mthumb")
