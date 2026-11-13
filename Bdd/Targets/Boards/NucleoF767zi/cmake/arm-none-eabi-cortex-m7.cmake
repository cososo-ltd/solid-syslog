# Cross toolchain for the NUCLEO-F767ZI: an STM32F767ZI, which is a Cortex-M7
# with a double-precision FPU. Selected by the nucleo-f767zi-* CMake presets.

include("${CMAKE_CURRENT_LIST_DIR}/../../../Common/cmake/arm-none-eabi-common.cmake")

# The core and the float ABI are global, not per target: every object in one
# image has to agree on how a floating-point argument is passed, and that
# includes libSolidSyslog.a, which is compiled outside any target's own
# options. The linker takes the same flags to pick the matching newlib.
set(SOLIDSYSLOG_NUCLEO_F767ZI_ARCH "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS_INIT   "${SOLIDSYSLOG_NUCLEO_F767ZI_ARCH} ${SOLIDSYSLOG_ARM_NONE_EABI_SECTION_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${SOLIDSYSLOG_NUCLEO_F767ZI_ARCH} ${SOLIDSYSLOG_ARM_NONE_EABI_SECTION_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${SOLIDSYSLOG_NUCLEO_F767ZI_ARCH}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${SOLIDSYSLOG_NUCLEO_F767ZI_ARCH}")
