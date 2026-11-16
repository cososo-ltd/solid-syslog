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
#   BDD_BOARD_SYSTEM_INCLUDES  - include directories for a vendor's headers,
#                                which the board's sources read and the
#                                target's upstreams may; SYSTEM for project
#                                code, so the vendor's style is not held to
#                                the project's warnings
#   BDD_BOARD_DEFINITIONS      - compile definitions the vendor's headers need
#                                wherever they are read
#   BDD_BOARD_UPSTREAM_SOURCES - the vendor's own sources - a HAL, a system
#                                file, startup code - compiled under the
#                                target's relaxed warnings
#
# A board that has no vendor layer leaves the last three empty.
#
# and, for the medium under each kind of filesystem a target may keep its store
# in, the sources a target of that kind adds to its executable:
#
#   BDD_BOARD_DISK_PROJECT_SOURCES      - the sector disk, which defines the
#                                         BddTargetBoardDisk_* functions, for a
#                                         filesystem that is kept in sectors
#   BDD_BOARD_LITTLEFS_PROJECT_SOURCES  - the LittleFS block device, which
#                                         defines BddTargetLittleFsDisk_Config
#                                         and BddTargetLittleFsDisk_Wipe, over
#                                         whichever medium the board has for it
#
# A target takes one of the two, so the board never needs to know which
# targets there are.
#
# and, for the Ethernet controller under each network stack a target may run:
#
#   BDD_BOARD_LWIP_PROJECT_SOURCES      - the netif that defines
#                                         BddTargetBoardNetif_Init
#   BDD_BOARD_LWIP_UPSTREAM_SOURCES     - third-party controller driver sources,
#                                         compiled under the target's relaxed
#                                         warnings
#   BDD_BOARD_LWIP_INCLUDES             - include directories for both
#   BDD_BOARD_PLUSTCP_PROJECT_SOURCES   - the file that defines
#                                         BddTargetBoardPlusTcp_FillInterface
#   BDD_BOARD_PLUSTCP_UPSTREAM_SOURCES  - the network interface and driver
#                                         upstream ships for the board, relative
#                                         to Plus-TCP's
#                                         source/portable/NetworkInterface
#   BDD_BOARD_PLUSTCP_UPSTREAM_INCLUDES - include directories for those,
#                                         relative to the same place
#   BDD_BOARD_PLUSTCP_UPSTREAM_DEFINITIONS - compile definitions those
#                                         sources need, and nothing else in
#                                         the image is given
#   BDD_BOARD_PLUSTCP_ZERO_COPY         - 1 where that interface hands the
#                                         stack's buffers to the controller as
#                                         they are, 0 where it copies each
#                                         frame
#   BDD_BOARD_PLUSTCP_BUFFER_ALLOCATION - which of Plus-TCP's two buffer
#                                         schemes that interface is written
#                                         for: 1, fixed buffers the
#                                         interface places, or 2, buffers
#                                         from the kernel's heap. An
#                                         interface that hands buffers to a
#                                         DMA needs them placed, so the two
#                                         are stated together
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
