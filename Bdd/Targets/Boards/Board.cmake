# The board half of a cross BDD target.
#
# A target's own directory holds what its RTOS, network stack and filesystem
# need; what belongs to the board it runs on sits in a directory beside this
# file, one per board, and the configure names which:
#
#   -DSOLIDSYSLOG_BDD_BOARD=<directory name>
#
# Each cross preset sets it. A target includes this file, which includes the
# chosen board's own Board.cmake, and takes from it:
#
#   BDD_BOARD_DIR              - the board's directory
#   BDD_BOARD_ARCH_FLAGS       - the core's compile and link flags, as a list
#   BDD_BOARD_FREERTOS_PORT    - the kernel port for the core, relative to the
#                                kernel's portable/ directory
#   BDD_BOARD_LINKER_SCRIPT    - the linker script
#   BDD_BOARD_PROJECT_SOURCES  - the board's own sources, compiled under the
#                                strict bar in the target's executable
#   BDD_BOARD_INCLUDES         - include directories for the headers the board
#                                supplies to the target and to its upstreams:
#                                BddTargetBoardConfig.h, the clock and NVIC
#                                facts the kernel configuration reads, and
#                                BddTargetCmsisDevice.h, the CMSIS device header
#
# The toolchain file is the board's too, since it fixes the core for every
# object in the image. It is named by the preset, not from here: CMake reads
# it before any of this runs.

set(SOLIDSYSLOG_BDD_BOARD "" CACHE STRING
    "Which board a cross BDD target is built for: a directory name under Bdd/Targets/Boards")

set(bdd_boards_dir "${CMAKE_CURRENT_LIST_DIR}")
if(NOT SOLIDSYSLOG_BDD_BOARD OR NOT EXISTS "${bdd_boards_dir}/${SOLIDSYSLOG_BDD_BOARD}/Board.cmake")
    file(GLOB bdd_board_files RELATIVE "${bdd_boards_dir}" "${bdd_boards_dir}/*/Board.cmake")
    list(TRANSFORM bdd_board_files REPLACE "/Board.cmake$" "")
    string(REPLACE ";" ", " bdd_board_list "${bdd_board_files}")
    message(FATAL_ERROR
        "SOLIDSYSLOG_BDD_BOARD must name a board under Bdd/Targets/Boards: ${bdd_board_list} "
        "(got: '${SOLIDSYSLOG_BDD_BOARD}'). The cross presets set it.")
endif()

include("${bdd_boards_dir}/${SOLIDSYSLOG_BDD_BOARD}/Board.cmake")
