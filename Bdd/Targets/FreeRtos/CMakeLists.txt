# FreeRTOS-Plus-TCP BDD target ELF for QEMU mps2-an385 (Cortex-M3).
#
# Compiled cross-only — gated by the top-level CMakeLists.txt on
# CMAKE_CROSSCOMPILING + CMAKE_SYSTEM_PROCESSOR == "arm".
#
# Wires SolidSyslogUdpSender behind the SolidSyslogFreeRtosDatagram adapter
# and the static resolver, with Bdd/Targets/Common/BddTargetInteractive
# driving `send N` / `quit` over the QEMU -serial stdio UART (CmsdkUart RX
# path).
#
# Source layout: upstream FreeRTOS-Kernel + Plus-TCP sources are compiled
# into a separate OBJECT library (`solid_syslog_freertos_upstream`) that
# accepts the relaxed warning set those headers / sources require. Local
# project code (main.c, Startup.c, Common/CmsdkUart.c, Common/Syscalls.c,
# the Platform/FreeRtos/Source adapters, and Bdd/Targets/Common/
# BddTargetInteractive.c) stays in the executable target under the strict
# project warning bar so regressions in our own code can't slip through.

set(FREERTOS_KERNEL_PATH "$ENV{FREERTOS_KERNEL_PATH}")
if(NOT FREERTOS_KERNEL_PATH)
    message(FATAL_ERROR
        "FREERTOS_KERNEL_PATH not set. Use the cpputest-freertos-cross "
        "container, which sets this to /opt/freertos/kernel.")
endif()

set(FREERTOS_PLUS_TCP_PATH "$ENV{FREERTOS_PLUS_TCP_PATH}")
if(NOT FREERTOS_PLUS_TCP_PATH)
    message(FATAL_ERROR
        "FREERTOS_PLUS_TCP_PATH not set. Use the cpputest-freertos-cross "
        "container, which sets this to /opt/freertos/plus-tcp.")
endif()

set(FATFS_PATH "$ENV{FATFS_PATH}")
if(NOT FATFS_PATH)
    message(FATAL_ERROR
        "FATFS_PATH not set. Use the cpputest-freertos-cross container, "
        "which sets this to /opt/fatfs.")
endif()

# Stage the upstream FatFs source pack alongside our integrator ffconf.h in
# the build dir. FatFs's ff.h does `#include "ffconf.h"` and GCC's `""`
# include lookup checks the source file's own directory first, so the only
# way our integrator config wins over upstream's is to colocate them.
# configure_file(COPYONLY) sets up the CMake dependency so reconfigure
# triggers on changes to either side.
set(FATFS_STAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/fatfs")
configure_file(${FATFS_PATH}/source/ff.c       ${FATFS_STAGE_DIR}/ff.c       COPYONLY)
configure_file(${FATFS_PATH}/source/ff.h       ${FATFS_STAGE_DIR}/ff.h       COPYONLY)
configure_file(${FATFS_PATH}/source/diskio.h   ${FATFS_STAGE_DIR}/diskio.h   COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ffconf.h ${FATFS_STAGE_DIR}/ffconf.h COPYONLY)

set(FREERTOS_PORT_DIR "${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM3")

set(PLUS_TCP_SRC_DIR "${FREERTOS_PLUS_TCP_PATH}/source")
set(PLUS_TCP_PORT_GCC_DIR "${PLUS_TCP_SRC_DIR}/portable/Compiler/GCC")
set(PLUS_TCP_BUF_DIR "${PLUS_TCP_SRC_DIR}/portable/BufferManagement")
set(PLUS_TCP_NIF_DIR "${PLUS_TCP_SRC_DIR}/portable/NetworkInterface/MPS2_AN385")

# --- Upstream OBJECT library --------------------------------------------------
#
# Compiles all kernel + Plus-TCP + LAN9118 driver sources with the relaxed
# warning set. Marked PRIVATE on this library; the executable consumes the
# objects via $<TARGET_OBJECTS:...> so the relaxations don't leak.

add_library(solid_syslog_freertos_upstream OBJECT
    ${FREERTOS_KERNEL_PATH}/tasks.c
    ${FREERTOS_KERNEL_PATH}/queue.c
    ${FREERTOS_KERNEL_PATH}/list.c
    ${FREERTOS_KERNEL_PATH}/timers.c
    ${FREERTOS_KERNEL_PATH}/event_groups.c
    ${FREERTOS_PORT_DIR}/port.c
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
    # Plus-TCP core (UDP-only — see FreeRTOSIPConfig.h).
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_ARP.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_BitConfig.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_DHCP.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_DHCPv6.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_DNS.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_DNS_Cache.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_DNS_Callback.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_DNS_Networking.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_DNS_Parser.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_ICMP.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_IP.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_IP_Timers.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_IP_Utils.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_IPv4.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_IPv4_Sockets.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_IPv4_Utils.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_IPv6.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_IPv6_Sockets.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_IPv6_Utils.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_ND.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_RA.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_Routing.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_Sockets.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_Stream_Buffer.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_IP.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_IP_IPv4.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_IP_IPv6.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_Reception.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_State_Handling.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_State_Handling_IPv4.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_State_Handling_IPv6.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_Transmission.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_Transmission_IPv4.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_Transmission_IPv6.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_Utils.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_Utils_IPv4.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_Utils_IPv6.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_TCP_WIN.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_Tiny_TCP.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_UDP_IP.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_UDP_IPv4.c
    ${PLUS_TCP_SRC_DIR}/FreeRTOS_UDP_IPv6.c
    ${PLUS_TCP_BUF_DIR}/BufferAllocation_2.c
    # MPS2_AN385 LAN9118 NetworkInterface + driver.
    ${PLUS_TCP_NIF_DIR}/NetworkInterface.c
    ${PLUS_TCP_NIF_DIR}/ether_lan9118/smsc9220_eth_drv.c
    # FatFs upstream (R0.16, FFCONF_DEF 80386). ff.c trips the same
    # sign-conversion / shadow / type-narrowing warnings the FreeRTOS
    # kernel sources do, so it lives in the upstream OBJECT lib under the
    # relaxed warning set. ffsystem.c is our integrator copy
    # (Bdd/Targets/FreeRtos/) with OS_TYPE 3 (FreeRTOS) selected — it sits
    # under the same relaxations because it includes ff.h and inherits
    # FatFs's typedef-via-BYTE/UINT patterns. ff.c is staged into the
    # build dir so its `#include "ff.h"` (and ff.h's "ffconf.h") resolve
    # to our integrator config — see the configure_file calls above.
    ${FATFS_STAGE_DIR}/ff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ffsystem.c
)

target_compile_options(solid_syslog_freertos_upstream PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -fno-common
    # FreeRTOS-Kernel + Plus-TCP V11.1.0/V4.2.2 use non-prototype function
    # declarations, type-narrowing constructs, sign-conversion patterns, and
    # shadowed locals that trip our strict host warnings. Scoped to this
    # OBJECT library so the relaxations never reach project code.
    -Wno-unused-parameter
    -Wno-unused-but-set-variable
    -Wno-implicit-fallthrough
    -Wno-array-bounds
    -Wno-conversion
    -Wno-sign-conversion
    -Wno-shadow
    -Wno-pedantic
)

target_include_directories(solid_syslog_freertos_upstream PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                       # FreeRTOSConfig.h, FreeRTOSIPConfig.h
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_PORT_DIR}                              # portmacro.h
    ${PLUS_TCP_SRC_DIR}/include
    ${PLUS_TCP_PORT_GCC_DIR}                          # pack_struct_*.h
    ${PLUS_TCP_NIF_DIR}                               # NetworkInterface.h consumed locally
    ${PLUS_TCP_NIF_DIR}/ether_lan9118                 # SMM_MPS2.h, smsc9220_*.h
    ${FATFS_STAGE_DIR}                                # ff.h, diskio.h, our ffconf.h colocated
)

# --- Executable ---------------------------------------------------------------
#
# Project code only. Inherits the full project warning set
# (-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror)
# from the top-level CMakeLists.txt — no -Wno-* relaxations here.

add_executable(SolidSyslogBddTarget
    main.c
    Startup.c
    diskio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/CmsdkUart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/Syscalls.c
    ${CMAKE_SOURCE_DIR}/Platform/FatFs/Source/SolidSyslogFatFsFile.c
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogAddress.c
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosDatagram.c
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosDatagramStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosMutex.c
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosMutexStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosStaticResolver.c
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosSysUpTime.c
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosTcpStream.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetInteractive.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetIps.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetLanguage.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetSwitchConfig.c
    $<TARGET_OBJECTS:solid_syslog_freertos_upstream>
)

set_target_properties(SolidSyslogBddTarget PROPERTIES SUFFIX ".elf")

target_compile_options(SolidSyslogBddTarget PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -fno-common
)

target_include_directories(SolidSyslogBddTarget PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                       # FreeRTOSConfig.h, FreeRTOSIPConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Common                # CmsdkUart.h
    ${CMAKE_SOURCE_DIR}/Core/Interface                # SolidSyslog*.h
    ${CMAKE_SOURCE_DIR}/Core/Source                   # SolidSyslogAddress.h (consumed by adapters)
    ${CMAKE_SOURCE_DIR}/Platform/FatFs/Interface      # SolidSyslogFatFsFile.h
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Interface   # SolidSyslogFreeRtos*.h
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source      # SolidSyslogAddressInternal.h
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common            # BddTargetInteractive.h
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_PORT_DIR}                              # portmacro.h
    ${PLUS_TCP_SRC_DIR}/include
    ${PLUS_TCP_PORT_GCC_DIR}                          # pack_struct_*.h
    ${FATFS_STAGE_DIR}                                # ff.h, diskio.h + integrator ffconf.h colocated — must match
                                                      # the OBJECT lib (above) so ff.c, main.c, diskio.c, and
                                                      # SolidSyslogFatFsFile.c all see the SAME ffconf.h. Pointing
                                                      # the executable at FATFS_PATH/source would resolve ff.h's
                                                      # `#include "ffconf.h"` to upstream's sample (same directory
                                                      # rule) and diverge FATFS / config-conditional layout between
                                                      # translation units.
)

target_link_libraries(SolidSyslogBddTarget PRIVATE
    ${PROJECT_NAME}                                   # libSolidSyslog.a (cross-compiled)
)

target_link_options(SolidSyslogBddTarget PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -T${CMAKE_CURRENT_SOURCE_DIR}/Common/mps2-an385.ld
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE:SolidSyslogBddTarget>.map
)

set_target_properties(SolidSyslogBddTarget PROPERTIES
    LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Common/mps2-an385.ld)
