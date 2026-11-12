# What every arm-none-eabi cross toolchain file here shares: the system, the
# tools and where CMake may look. Each toolchain file includes this and adds
# only its own core's architecture flags.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# A bare-metal executable needs a linker script and startup code. CMake's
# compiler probe links a default executable; have it probe with a static
# library instead, so the probe needs neither.
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

# Each function and object in its own section, so the link can drop what
# nothing references.
set(SOLIDSYSLOG_ARM_NONE_EABI_SECTION_FLAGS "-ffunction-sections -fdata-sections -fno-common")
