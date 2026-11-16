# The QEMU mps2-an385 machine: a Cortex-M3 with CMSDK UARTs and a LAN9118
# Ethernet controller. See ../Board.cmake for what each name below is for.

set(BDD_BOARD_DIR ${CMAKE_CURRENT_LIST_DIR})

# Any arm toolchain file passes the top-level check, and one for another core
# would build an image the machine cannot run.
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
    ${BDD_BOARD_DIR}/BddTargetBoard.c
    ${BDD_BOARD_DIR}/BddTargetBoardClock.c
    ${BDD_BOARD_DIR}/CmsdkUart.c
)

# The machine's only medium is a flat image on the QEMU host. It is the
# sector disk, and LittleFS gets a block device over those sectors.
set(BDD_BOARD_DISK_PROJECT_SOURCES
    ${BDD_BOARD_DIR}/BddTargetBoardDisk.c
)
set(BDD_BOARD_LITTLEFS_PROJECT_SOURCES
    ${BDD_BOARD_DISK_PROJECT_SOURCES}
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetLittleFsDisk.c
)

set(BDD_BOARD_INCLUDES
    ${BDD_BOARD_DIR}                                  # BddTargetBoardConfig.h, BddTargetCmsisDevice.h
)

# The machine has no vendor layer: its startup and drivers are in this
# directory.
set(BDD_BOARD_SYSTEM_INCLUDES)
set(BDD_BOARD_DEFINITIONS)
set(BDD_BOARD_UPSTREAM_SOURCES)

# The LAN9118 under lwIP: our netif over the vendored Arm controller driver
# (Apache-2.0), compiled under the relaxed warnings as third-party source.
set(BDD_BOARD_LWIP_PROJECT_SOURCES
    ${BDD_BOARD_DIR}/Lwip/BddTargetBoardNetif.c
)
set(BDD_BOARD_LWIP_UPSTREAM_SOURCES
    ${BDD_BOARD_DIR}/Lwip/smsc9220/smsc9220_eth_drv.c
)
set(BDD_BOARD_LWIP_INCLUDES
    ${BDD_BOARD_DIR}/Lwip/smsc9220                    # smsc9220_eth_drv.h, smsc9220_emac_config.h
)

# The LAN9118 under FreeRTOS-Plus-TCP: upstream's own network interface and
# controller driver for this machine, and our file that fills the descriptor.
set(BDD_BOARD_PLUSTCP_PROJECT_SOURCES
    ${BDD_BOARD_DIR}/PlusTcp/BddTargetBoardPlusTcp.c
)
set(BDD_BOARD_PLUSTCP_UPSTREAM_SOURCES
    MPS2_AN385/NetworkInterface.c
    MPS2_AN385/ether_lan9118/smsc9220_eth_drv.c
)
set(BDD_BOARD_PLUSTCP_UPSTREAM_INCLUDES
    MPS2_AN385                                        # NetworkInterface.h consumed locally
    MPS2_AN385/ether_lan9118                          # SMM_MPS2.h, smsc9220_*.h
)
set(BDD_BOARD_PLUSTCP_UPSTREAM_DEFINITIONS)
# That interface copies each frame, and takes its buffers from the heap.
set(BDD_BOARD_PLUSTCP_ZERO_COPY 0)
set(BDD_BOARD_PLUSTCP_BUFFER_ALLOCATION 2)
