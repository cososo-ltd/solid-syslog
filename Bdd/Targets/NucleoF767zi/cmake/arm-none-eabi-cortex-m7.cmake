# Cross toolchain for the NUCLEO-F767ZI: an STM32F767ZI, which is a Cortex-M7
# with a double-precision FPU. Selected by the nucleo-f767zi-* CMake presets.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# A bare-metal executable needs the board's linker script and startup code.
# CMake's compiler probe links a default executable; have it probe with a
# static library instead, so the probe needs neither.
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

# The core and the float ABI are global, not per target: every object in one
# image has to agree on how a floating-point argument is passed, and that
# includes libSolidSyslog.a, which is compiled outside any target's own
# options. The linker takes the same flags to pick the matching newlib.
set(SOLIDSYSLOG_NUCLEO_F767ZI_ARCH "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS_INIT   "${SOLIDSYSLOG_NUCLEO_F767ZI_ARCH} -ffunction-sections -fdata-sections -fno-common")
set(CMAKE_CXX_FLAGS_INIT "${SOLIDSYSLOG_NUCLEO_F767ZI_ARCH} -ffunction-sections -fdata-sections -fno-common")
set(CMAKE_ASM_FLAGS_INIT "${SOLIDSYSLOG_NUCLEO_F767ZI_ARCH}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${SOLIDSYSLOG_NUCLEO_F767ZI_ARCH}")
