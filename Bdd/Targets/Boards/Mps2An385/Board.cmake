# The QEMU mps2-an385 machine: a Cortex-M3 with CMSDK UARTs and a LAN9118
# Ethernet controller. See ../Board.cmake for what each name below is for.

set(BDD_BOARD_DIR ${CMAKE_CURRENT_LIST_DIR})

# Any arm toolchain file passes the top-level gate, and one for another core
# would compile this without complaint into an image the machine cannot run.
if(NOT CMAKE_C_FLAGS MATCHES "-mcpu=cortex-m3")
    message(FATAL_ERROR
        "The mps2-an385 board needs its own toolchain file, "
        "Bdd/Targets/Boards/Mps2An385/cmake/arm-none-eabi.cmake. "
        "Configure with one of the freertos-cross or cmsis-cross presets.")
endif()

set(BDD_BOARD_ARCH_FLAGS -mcpu=cortex-m3 -mthumb)

set(BDD_BOARD_FREERTOS_PORT GCC/ARM_CM3)

set(BDD_BOARD_LINKER_SCRIPT ${BDD_BOARD_DIR}/mps2-an385.ld)

set(BDD_BOARD_PROJECT_SOURCES
    ${BDD_BOARD_DIR}/Startup.c
)

set(BDD_BOARD_INCLUDES
    ${BDD_BOARD_DIR}                                  # Mps2An385.h
)
