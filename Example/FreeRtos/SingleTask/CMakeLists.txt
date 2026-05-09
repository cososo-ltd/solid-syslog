# FreeRTOS-Plus-TCP single-task UDP smoke ELF for QEMU mps2-an385 (Cortex-M3).
#
# Compiled cross-only — gated by the top-level CMakeLists.txt on
# CMAKE_CROSSCOMPILING + CMAKE_SYSTEM_PROCESSOR == "arm".
#
# Slice 3b.1 of S08.03 stands the IP stack up and emits a single UDP
# datagram on link-up. Slice 3b.2 swaps main.c's body for SolidSyslog wiring;
# the directory + CMakeLists stay.
#
# Source layout: upstream FreeRTOS-Kernel + Plus-TCP sources are compiled
# into a separate OBJECT library (`solid_syslog_freertos_upstream`) that
# accepts the relaxed warning set those headers / sources require. Local
# project code (main.c, Startup.c, Common/CmsdkUart.c, Common/Syscalls.c)
# stays in the executable target under the strict project warning bar so
# regressions in our own code can't slip through.

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
)

# --- Executable ---------------------------------------------------------------
#
# Project code only. Inherits the full project warning set
# (-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror)
# from the top-level CMakeLists.txt — no -Wno-* relaxations here.

add_executable(SolidSyslogFreeRtosSingleTask
    main.c
    Startup.c
    ${SOLID_SYSLOG_FREERTOS_EXAMPLE_COMMON_DIR}/CmsdkUart.c
    ${SOLID_SYSLOG_FREERTOS_EXAMPLE_COMMON_DIR}/Syscalls.c
    $<TARGET_OBJECTS:solid_syslog_freertos_upstream>
)

set_target_properties(SolidSyslogFreeRtosSingleTask PROPERTIES SUFFIX ".elf")

target_compile_options(SolidSyslogFreeRtosSingleTask PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -fno-common
)

target_include_directories(SolidSyslogFreeRtosSingleTask PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                       # FreeRTOSConfig.h, FreeRTOSIPConfig.h
    ${SOLID_SYSLOG_FREERTOS_EXAMPLE_COMMON_DIR}       # CmsdkUart.h
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_PORT_DIR}                              # portmacro.h
    ${PLUS_TCP_SRC_DIR}/include
    ${PLUS_TCP_PORT_GCC_DIR}                          # pack_struct_*.h
)

target_link_options(SolidSyslogFreeRtosSingleTask PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -T${SOLID_SYSLOG_FREERTOS_EXAMPLE_COMMON_DIR}/mps2-an385.ld
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE:SolidSyslogFreeRtosSingleTask>.map
)

set_target_properties(SolidSyslogFreeRtosSingleTask PROPERTIES
    LINK_DEPENDS ${SOLID_SYSLOG_FREERTOS_EXAMPLE_COMMON_DIR}/mps2-an385.ld)
