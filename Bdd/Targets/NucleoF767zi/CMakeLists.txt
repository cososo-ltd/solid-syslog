# Firmware for the NUCLEO-F767ZI (STM32F767ZI, Cortex-M7).
#
# Compiled cross-only - gated by the top-level CMakeLists.txt on
# CMAKE_CROSSCOMPILING + CMAKE_SYSTEM_PROCESSOR == "arm", and selected by
# SOLIDSYSLOG_BDD_TARGET. Board/ is the board support: the linker script, the
# HAL configuration, and the clock, console and LEDs over ST's HAL. Bringup/ is
# the one image built so far, which carries no SolidSyslog.
#
# ST's HAL driver and CMSIS device files compile into an OBJECT library under
# a relaxed warning set, as the other targets' upstream sources do; the code
# in this directory stays under the project's own warning bar.

# The upstream trees this target wires - see Bdd/Targets/Common/UpstreamTrees.cmake.
solidsyslog_bdd_upstream(STM32F7_HAL_DRIVER_PATH SOLIDSYSLOG_STM32F7_HAL_DRIVER_PATH /opt/stm32/f7-hal-driver
    PROBE Inc/stm32f7xx_hal.h)
solidsyslog_bdd_upstream(STM32F7_CMSIS_DEVICE_PATH SOLIDSYSLOG_STM32F7_CMSIS_DEVICE_PATH /opt/stm32/f7-cmsis-device
    PROBE Include/stm32f767xx.h)
solidsyslog_bdd_upstream(CMSIS_PATH SOLIDSYSLOG_CMSIS_PATH /opt/cmsis PROBE CMSIS/Core/Include/core_cm7.h)

# The startup code is ST's, in assembler.
enable_language(ASM)

set(BOARD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Board")
set(BOARD_LINKER_SCRIPT "${BOARD_DIR}/stm32f767zi.ld")

# The part, which selects its register definitions in ST's headers, and the
# switch that makes those headers pull in the HAL.
set(BOARD_DEFINITIONS STM32F767xx USE_HAL_DRIVER)

set(BOARD_UPSTREAM_INCLUDES
    ${STM32F7_HAL_DRIVER_PATH}/Inc
    ${STM32F7_CMSIS_DEVICE_PATH}/Include
    ${CMSIS_PATH}/CMSIS/Core/Include
)

# --- Upstream OBJECT library --------------------------------------------------
#
# One HAL source per module stm32f7xx_hal_conf.h enables, plus the extension
# files those modules call into, the device's system file and its startup code.

set(HAL_SOURCE_DIR "${STM32F7_HAL_DRIVER_PATH}/Src")

add_library(solid_syslog_nucleo_f767zi_upstream OBJECT
    ${HAL_SOURCE_DIR}/stm32f7xx_hal.c
    ${HAL_SOURCE_DIR}/stm32f7xx_hal_cortex.c
    ${HAL_SOURCE_DIR}/stm32f7xx_hal_dma.c
    ${HAL_SOURCE_DIR}/stm32f7xx_hal_gpio.c
    ${HAL_SOURCE_DIR}/stm32f7xx_hal_pwr.c
    ${HAL_SOURCE_DIR}/stm32f7xx_hal_pwr_ex.c
    ${HAL_SOURCE_DIR}/stm32f7xx_hal_rcc.c
    ${HAL_SOURCE_DIR}/stm32f7xx_hal_rcc_ex.c
    ${HAL_SOURCE_DIR}/stm32f7xx_hal_uart.c
    ${HAL_SOURCE_DIR}/stm32f7xx_hal_uart_ex.c
    ${STM32F7_CMSIS_DEVICE_PATH}/Source/Templates/system_stm32f7xx.c
    ${STM32F7_CMSIS_DEVICE_PATH}/Source/Templates/gcc/startup_stm32f767xx.s
)

target_compile_definitions(solid_syslog_nucleo_f767zi_upstream PRIVATE ${BOARD_DEFINITIONS})

# ST's sources leave parameters unused and convert between widths freely.
# Scoped to this OBJECT library so the relaxations never reach project code.
target_compile_options(solid_syslog_nucleo_f767zi_upstream PRIVATE
    -Wno-unused-parameter
    -Wno-conversion
    -Wno-sign-conversion
    -Wno-pedantic
)

target_include_directories(solid_syslog_nucleo_f767zi_upstream PRIVATE
    ${BOARD_DIR}                                      # stm32f7xx_hal_conf.h
    ${BOARD_UPSTREAM_INCLUDES}
)

# --- Bring-up image -----------------------------------------------------------
#
# Project code only, under the full project warning set. ST's headers are
# SYSTEM includes: Board.c has to read them, and their macros expand in our
# code, but their own style is not ours to hold to our warnings.

# The commit goes into the banner, so the checklist can tell which build is on
# the board. It is the commit checked out, which on a pull request is CI's
# merge of the branch into main. Read at configure time, which is every time
# in CI.
find_package(Git QUIET)
set(BRINGUP_BUILD_COMMIT "unknown")
if(GIT_EXECUTABLE)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_SOURCE_DIR}" rev-parse --short=10 HEAD
        OUTPUT_VARIABLE bringup_commit OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE bringup_commit_result ERROR_QUIET)
    if(bringup_commit_result EQUAL 0)
        set(BRINGUP_BUILD_COMMIT "${bringup_commit}")
    endif()
endif()

add_executable(SolidSyslogNucleoF767ziBringup
    Bringup/main.c
    Board/Board.c
    $<TARGET_OBJECTS:solid_syslog_nucleo_f767zi_upstream>
)

set_target_properties(SolidSyslogNucleoF767ziBringup PROPERTIES SUFFIX ".elf")

target_compile_definitions(SolidSyslogNucleoF767ziBringup PRIVATE
    ${BOARD_DEFINITIONS}
    BRINGUP_BUILD_COMMIT="${BRINGUP_BUILD_COMMIT}"
)

target_include_directories(SolidSyslogNucleoF767ziBringup PRIVATE
    ${BOARD_DIR}                                      # Board.h, stm32f7xx_hal_conf.h
)
target_include_directories(SolidSyslogNucleoF767ziBringup SYSTEM PRIVATE
    ${BOARD_UPSTREAM_INCLUDES}
)

target_link_options(SolidSyslogNucleoF767ziBringup PRIVATE
    -T${BOARD_LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE:SolidSyslogNucleoF767ziBringup>.map
)

set_target_properties(SolidSyslogNucleoF767ziBringup PROPERTIES
    LINK_DEPENDS ${BOARD_LINKER_SCRIPT})

# The flash tool takes the ELF, which carries its own load address. The raw
# binary is built beside it for a tool that wants one, to be written at the
# start of flash, 0x08000000.
add_custom_command(TARGET SolidSyslogNucleoF767ziBringup POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
        $<TARGET_FILE:SolidSyslogNucleoF767ziBringup>
        $<TARGET_FILE_DIR:SolidSyslogNucleoF767ziBringup>/SolidSyslogNucleoF767ziBringup.bin
    COMMENT "Extracting SolidSyslogNucleoF767ziBringup.bin"
    VERBATIM
)
