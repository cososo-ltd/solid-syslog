# Cross toolchain for the QEMU mps2-an385 machine, a Cortex-M3. Selected by the
# freertos-cross and cmsis-cross CMake presets.

include("${CMAKE_CURRENT_LIST_DIR}/../../../Common/cmake/arm-none-eabi-common.cmake")

# Architecture flags applied globally so libSolidSyslog.a (compiled outside
# the executable target's PRIVATE compile_options) emits Thumb-2 instructions
# the Cortex-M3 can decode. arm-none-eabi-gcc otherwise defaults to ARM mode
# for armv7+, which links cleanly but hard-faults on the first call from
# Thumb code (slice 3b.2 hit this on SolidSyslogUdpSender_Create). The board's
# BDD_BOARD_ARCH_FLAGS repeat them per target, which is idempotent.
set(CMAKE_C_FLAGS_INIT   "-mcpu=cortex-m3 -mthumb ${SOLIDSYSLOG_ARM_NONE_EABI_SECTION_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m3 -mthumb ${SOLIDSYSLOG_ARM_NONE_EABI_SECTION_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m3 -mthumb")
