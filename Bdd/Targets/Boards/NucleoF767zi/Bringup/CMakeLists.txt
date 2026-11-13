# The bring-up firmware for the NUCLEO-F767ZI (STM32F767ZI, Cortex-M7).
#
# Compiled cross-only - gated by the top-level CMakeLists.txt on
# CMAKE_CROSSCOMPILING + CMAKE_SYSTEM_PROCESSOR == "arm", and selected by
# SOLIDSYSLOG_BDD_TARGET. The directory above is the board support, which the
# BDD targets reach through the board seam; this image takes it directly, and
# carries no SolidSyslog, no kernel and no network stack. It is what tells a
# fault in the board or its toolchain from one in anything built on them.
#
# ST's HAL driver and CMSIS device files compile into an OBJECT library under
# a relaxed warning set, as the other targets' upstream sources do; the code
# in this directory stays under the project's own warning bar.

# What the board supplies - the same list the BDD targets build from.
include(${CMAKE_CURRENT_SOURCE_DIR}/../Board.cmake)

# --- Upstream OBJECT library --------------------------------------------------

add_library(solid_syslog_nucleo_f767zi_upstream OBJECT ${BDD_BOARD_UPSTREAM_SOURCES})

target_compile_definitions(solid_syslog_nucleo_f767zi_upstream PRIVATE ${BDD_BOARD_DEFINITIONS})

# ST's sources leave parameters unused and convert between widths freely.
# Scoped to this OBJECT library so the relaxations never reach project code.
target_compile_options(solid_syslog_nucleo_f767zi_upstream PRIVATE
    -Wno-unused-parameter
    -Wno-conversion
    -Wno-sign-conversion
    -Wno-pedantic
)

target_include_directories(solid_syslog_nucleo_f767zi_upstream PRIVATE
    ${BDD_BOARD_DIR}                                  # stm32f7xx_hal_conf.h
    ${BDD_BOARD_SYSTEM_INCLUDES}
)

# --- Bring-up image -----------------------------------------------------------
#
# Project code only, under the full project warning set. ST's headers are
# SYSTEM includes: Board.c has to read them, and their macros expand in our
# code, but their own style is not ours to hold to our warnings.

add_executable(SolidSyslogNucleoF767ziBringup
    main.c
    ${BDD_BOARD_DIR}/Board.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetLineAssembler.c
    $<TARGET_OBJECTS:solid_syslog_nucleo_f767zi_upstream>
)

set_target_properties(SolidSyslogNucleoF767ziBringup PROPERTIES SUFFIX ".elf")

target_compile_definitions(SolidSyslogNucleoF767ziBringup PRIVATE ${BDD_BOARD_DEFINITIONS})

# The banner is this image's own, so the commit in it is given to the one file
# that prints it.
set_source_files_properties(main.c PROPERTIES
    COMPILE_DEFINITIONS "${BDD_BOARD_BUILD_COMMIT_DEFINITION}")

target_include_directories(SolidSyslogNucleoF767ziBringup PRIVATE
    ${BDD_BOARD_DIR}                                  # Board.h, stm32f7xx_hal_conf.h
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common            # BddTargetLineAssembler.h
    ${CMAKE_SOURCE_DIR}/Core/Interface                # SolidSyslogExternC.h, which that header reads
)
target_include_directories(SolidSyslogNucleoF767ziBringup SYSTEM PRIVATE
    ${BDD_BOARD_SYSTEM_INCLUDES}
)

target_link_options(SolidSyslogNucleoF767ziBringup PRIVATE
    -T${BDD_BOARD_LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE:SolidSyslogNucleoF767ziBringup>.map
)

set_target_properties(SolidSyslogNucleoF767ziBringup PROPERTIES
    LINK_DEPENDS ${BDD_BOARD_LINKER_SCRIPT})

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
