# ST's NUCLEO-F767ZI: an STM32F767ZI, which is a Cortex-M7 with a
# double-precision FPU and an Ethernet MAC, wired to an on-board LAN8742A PHY.
# See ../Board.cmake for what each name below is for.

set(BDD_BOARD_DIR ${CMAKE_CURRENT_LIST_DIR})

# Any arm toolchain file passes the top-level gate, and one for another core
# would compile this without complaint into an image the part cannot run.
if(NOT CMAKE_C_FLAGS MATCHES "-mcpu=cortex-m7")
    message(FATAL_ERROR
        "The NUCLEO-F767ZI board needs its own toolchain file, "
        "Bdd/Targets/Boards/NucleoF767zi/cmake/arm-none-eabi-cortex-m7.cmake. "
        "Configure with a nucleo-f767zi-* preset.")
endif()

# The upstream trees the board wires - see Bdd/Targets/Common/UpstreamTrees.cmake.
solidsyslog_bdd_upstream(STM32F7_HAL_DRIVER_PATH SOLIDSYSLOG_STM32F7_HAL_DRIVER_PATH /opt/stm32/f7-hal-driver
    PROBE Inc/stm32f7xx_hal.h)
solidsyslog_bdd_upstream(STM32F7_CMSIS_DEVICE_PATH SOLIDSYSLOG_STM32F7_CMSIS_DEVICE_PATH /opt/stm32/f7-cmsis-device
    PROBE Include/stm32f767xx.h)
solidsyslog_bdd_upstream(CMSIS_PATH SOLIDSYSLOG_CMSIS_PATH /opt/cmsis PROBE CMSIS/Core/Include/core_cm7.h)

# The startup code is ST's, in assembler.
enable_language(ASM)

# The toolchain file already gives every object these, because the float ABI
# has to agree across the whole image. Stated again so a target that passes the
# board's flags to its link gets the matching newlib.
set(BDD_BOARD_ARCH_FLAGS -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)

set(BDD_BOARD_FREERTOS_PORT GCC/ARM_CM7/r0p1)

set(BDD_BOARD_LINKER_SCRIPT ${BDD_BOARD_DIR}/stm32f767zi.ld)

# --- Addresses ----------------------------------------------------------------
#
# The board takes a static address, and where it sits is a fact about the
# network it is plugged into, not about the board. Each is a cache variable, so
# a build for a real network names its own:
#
#   cmake --preset nucleo-f767zi-lwip -DSOLIDSYSLOG_BDD_BOARD_IP=... \
#       -DSOLIDSYSLOG_BDD_BOARD_ORACLE=...
#
# The defaults are from 192.0.2.0/24, the block RFC 5737 reserves for
# documentation, so an image built without them - CI's - reaches nothing.

set(SOLIDSYSLOG_BDD_BOARD_IP "192.0.2.2" CACHE STRING
    "The NUCLEO-F767ZI's own IPv4 address")
set(SOLIDSYSLOG_BDD_BOARD_NETMASK "255.255.255.0" CACHE STRING
    "The NUCLEO-F767ZI's IPv4 netmask")
set(SOLIDSYSLOG_BDD_BOARD_GATEWAY "192.0.2.1" CACHE STRING
    "The NUCLEO-F767ZI's IPv4 gateway")
set(SOLIDSYSLOG_BDD_BOARD_ORACLE "192.0.2.1" CACHE STRING
    "The IPv4 address the NUCLEO-F767ZI reaches the syslog collector at")
set(SOLIDSYSLOG_BDD_BOARD_MAC "02:00:00:00:00:01" CACHE STRING
    "The NUCLEO-F767ZI's Ethernet hardware address")

# Sets <output> to the four octets of a dotted IPv4 address as the initialiser
# list a network stack's own macros take: "192U, 0U, 2U, 2U".
function(bdd_board_ipv4_octets output variable)
    set(address "${${variable}}")
    if(NOT address MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
        message(FATAL_ERROR "${variable} must be a dotted IPv4 address (got: '${address}').")
    endif()
    set(octets ${CMAKE_MATCH_1} ${CMAKE_MATCH_2} ${CMAKE_MATCH_3} ${CMAKE_MATCH_4})
    foreach(octet IN LISTS octets)
        if(octet GREATER 255)
            message(FATAL_ERROR "${variable} must be a dotted IPv4 address (got: '${address}').")
        endif()
    endforeach()
    list(TRANSFORM octets APPEND "U")
    list(JOIN octets ", " joined)
    set(${output} "${joined}" PARENT_SCOPE)
endfunction()

bdd_board_ipv4_octets(BDD_BOARD_IP_OCTETS SOLIDSYSLOG_BDD_BOARD_IP)
bdd_board_ipv4_octets(BDD_BOARD_NETMASK_OCTETS SOLIDSYSLOG_BDD_BOARD_NETMASK)
bdd_board_ipv4_octets(BDD_BOARD_GATEWAY_OCTETS SOLIDSYSLOG_BDD_BOARD_GATEWAY)
bdd_board_ipv4_octets(BDD_BOARD_ORACLE_OCTETS SOLIDSYSLOG_BDD_BOARD_ORACLE)

set(bdd_board_hex "[0-9A-Fa-f][0-9A-Fa-f]")
if(NOT SOLIDSYSLOG_BDD_BOARD_MAC MATCHES
   "^${bdd_board_hex}:${bdd_board_hex}:${bdd_board_hex}:${bdd_board_hex}:${bdd_board_hex}:${bdd_board_hex}$")
    message(FATAL_ERROR
        "SOLIDSYSLOG_BDD_BOARD_MAC must be six hexadecimal octets separated by colons "
        "(got: '${SOLIDSYSLOG_BDD_BOARD_MAC}').")
endif()
string(REPLACE ":" ";" bdd_board_mac_octets "${SOLIDSYSLOG_BDD_BOARD_MAC}")
list(TRANSFORM bdd_board_mac_octets PREPEND "0x")
list(TRANSFORM bdd_board_mac_octets APPEND "U")
list(JOIN bdd_board_mac_octets ", " BDD_BOARD_MAC_OCTETS)

set(BDD_BOARD_ORACLE_ADDRESS "${SOLIDSYSLOG_BDD_BOARD_ORACLE}")

# BddTargetBoardConfig.h reads the addresses from this header, written into the
# build tree so that no address is ever in a file the repository holds.
set(BDD_BOARD_GENERATED_DIR "${CMAKE_BINARY_DIR}/Boards/NucleoF767zi")
configure_file(
    ${BDD_BOARD_DIR}/BddTargetBoardAddresses.h.in
    ${BDD_BOARD_GENERATED_DIR}/BddTargetBoardAddresses.h
    @ONLY)

# --- Build commit -------------------------------------------------------------
#
# The commit goes into the banner, so the checklist can tell which build is on
# the board. It is the commit checked out, which on a pull request is CI's
# merge of the branch into main. Read at configure time, which is every time
# in CI.
find_package(Git QUIET)
set(BDD_BOARD_BUILD_COMMIT "unknown")
if(GIT_EXECUTABLE)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_SOURCE_DIR}" rev-parse --short=10 HEAD
        OUTPUT_VARIABLE bdd_board_commit OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE bdd_board_commit_result ERROR_QUIET)
    if(bdd_board_commit_result EQUAL 0)
        set(BDD_BOARD_BUILD_COMMIT "${bdd_board_commit}")
    endif()
endif()

# --- What the board supplies ---------------------------------------------------

# The part, which selects its register definitions in ST's headers, and the
# switch that makes those headers pull in the HAL.
set(BDD_BOARD_DEFINITIONS
    STM32F767xx
    USE_HAL_DRIVER
    BDD_TARGET_BOARD_BUILD_COMMIT="${BDD_BOARD_BUILD_COMMIT}"
)

set(BDD_BOARD_PROJECT_SOURCES
    ${BDD_BOARD_DIR}/Board.c
    ${BDD_BOARD_DIR}/BddTargetBoard.c
    ${BDD_BOARD_DIR}/BddTargetBoardDisk.c
)

set(BDD_BOARD_INCLUDES
    ${BDD_BOARD_DIR}                                  # BddTargetBoardConfig.h, Board.h, stm32f7xx_hal_conf.h
    ${BDD_BOARD_GENERATED_DIR}                        # BddTargetBoardAddresses.h
)

# ST's headers. Board code has to read them, and their macros expand in it, but
# their own style is not ours to hold to our warnings.
set(BDD_BOARD_SYSTEM_INCLUDES
    ${STM32F7_HAL_DRIVER_PATH}/Inc
    ${STM32F7_CMSIS_DEVICE_PATH}/Include
    ${CMSIS_PATH}/CMSIS/Core/Include
)

# One HAL source per module stm32f7xx_hal_conf.h enables, plus the extension
# files those modules call into, the device's system file and its startup code.
set(bdd_board_hal_source_dir "${STM32F7_HAL_DRIVER_PATH}/Src")
set(BDD_BOARD_UPSTREAM_SOURCES
    ${bdd_board_hal_source_dir}/stm32f7xx_hal.c
    ${bdd_board_hal_source_dir}/stm32f7xx_hal_cortex.c
    ${bdd_board_hal_source_dir}/stm32f7xx_hal_dma.c
    ${bdd_board_hal_source_dir}/stm32f7xx_hal_flash.c
    ${bdd_board_hal_source_dir}/stm32f7xx_hal_flash_ex.c
    ${bdd_board_hal_source_dir}/stm32f7xx_hal_gpio.c
    ${bdd_board_hal_source_dir}/stm32f7xx_hal_pwr.c
    ${bdd_board_hal_source_dir}/stm32f7xx_hal_pwr_ex.c
    ${bdd_board_hal_source_dir}/stm32f7xx_hal_rcc.c
    ${bdd_board_hal_source_dir}/stm32f7xx_hal_rcc_ex.c
    ${bdd_board_hal_source_dir}/stm32f7xx_hal_uart.c
    ${bdd_board_hal_source_dir}/stm32f7xx_hal_uart_ex.c
    ${STM32F7_CMSIS_DEVICE_PATH}/Source/Templates/system_stm32f7xx.c
    ${STM32F7_CMSIS_DEVICE_PATH}/Source/Templates/gcc/startup_stm32f767xx.s
)

# The Ethernet MAC under lwIP: our netif. No media is fitted yet, and no
# FreeRTOS-Plus-TCP interface is written, so those names stay empty.
set(BDD_BOARD_LWIP_PROJECT_SOURCES
    ${BDD_BOARD_DIR}/Lwip/BddTargetBoardNetif.c
)
set(BDD_BOARD_LWIP_UPSTREAM_SOURCES)
set(BDD_BOARD_LWIP_INCLUDES)
