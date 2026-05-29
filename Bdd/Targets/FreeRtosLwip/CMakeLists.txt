# FreeRTOS + lwIP (Raw API, NO_SYS=0) BDD target ELF for QEMU mps2-an385
# (Cortex-M3).
#
# S28.09: grew the S28.07 link-probe into a runnable BDD target — a real
# LAN9118 lwIP netif (netif/EthernetIf.c over the vendored Arm smsc9220
# driver), tcpip_init + a static IP, and the SolidSyslog UDP pipeline reaching
# the syslog-ng oracle on QEMU. Selected via SOLIDSYSLOG_FREERTOS_NET=LWIP,
# with zero dependency on Platform/PlusTcp / FreeRTOS-Plus-TCP.
#
# Source layout mirrors Bdd/Targets/FreeRtos/CMakeLists.txt: upstream
# FreeRTOS-Kernel + lwIP-core + lwIP-api + the contrib FreeRTOS sys_arch + the
# vendored LAN9118 driver compile into a separate OBJECT library
# (`solid_syslog_freertos_lwip_upstream`) under a relaxed warning set; our own
# code (main.c, the reused Cortex-M3 startup, netif/EthernetIf.c, the
# Platform/LwipRaw + Platform/FreeRtos adapters, and the shared BddTarget
# sources) stays in the executable under the strict project warning bar.

set(FREERTOS_KERNEL_PATH "$ENV{FREERTOS_KERNEL_PATH}")
if(NOT FREERTOS_KERNEL_PATH)
    message(FATAL_ERROR
        "FREERTOS_KERNEL_PATH not set. Use the cpputest-freertos-cross "
        "container, which sets this to /opt/freertos/kernel.")
endif()

set(LWIP_DIR "$ENV{LWIP_PATH}")
if(NOT LWIP_DIR)
    message(FATAL_ERROR
        "LWIP_PATH not set. Use the cpputest-freertos-cross container, "
        "which sets this to /opt/lwip.")
endif()

# lwIP ships its source lists as *_SRCS variables via Filelists.cmake. For
# NO_SYS=0 we consume the IPv4 core set plus the api/ set (tcpip.c, api_lib,
# api_msg, …) — the tcpip thread, timeouts, and tcpip_callback marshal hop
# live there. No ipv6.
include(${LWIP_DIR}/src/Filelists.cmake)

set(LWIP_CONTRIB_FREERTOS_DIR "${LWIP_DIR}/contrib/ports/freertos")

set(FREERTOS_PORT_DIR "${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM3")
set(FREERTOS_TCP_TARGET_DIR "${CMAKE_SOURCE_DIR}/Bdd/Targets/FreeRtos")
set(FREERTOS_COMMON_DIR "${FREERTOS_TCP_TARGET_DIR}/Common")

# --- Upstream OBJECT library --------------------------------------------------
#
# FreeRTOS kernel + lwIP IPv4 core + lwIP api (tcpip thread) + the contrib
# FreeRTOS sys_arch + the vendored Arm LAN9118 driver, compiled with the
# relaxed warning set those upstream / third-party sources require. The
# executable consumes the objects via $<TARGET_OBJECTS:...> so the relaxations
# don't leak into project code.

add_library(solid_syslog_freertos_lwip_upstream OBJECT
    ${FREERTOS_KERNEL_PATH}/tasks.c
    ${FREERTOS_KERNEL_PATH}/queue.c
    ${FREERTOS_KERNEL_PATH}/list.c
    ${FREERTOS_KERNEL_PATH}/timers.c
    ${FREERTOS_KERNEL_PATH}/event_groups.c
    ${FREERTOS_PORT_DIR}/port.c
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
    ${lwipcore_SRCS}
    ${lwipcore4_SRCS}
    ${lwipapi_SRCS}
    # etharp (in core4) emits ARP frames via ethernet_output/ethbroadcast/
    # ethzero from netif/ethernet.c — pull that one netif source in (but not
    # bridgeif/slipif from lwipnetif_SRCS, which this target has no use for).
    ${LWIP_DIR}/src/netif/ethernet.c
    # lwIP FreeRTOS OS-abstraction port (sys_mutex/sem/mbox/thread) — required
    # for NO_SYS=0.
    ${LWIP_CONTRIB_FREERTOS_DIR}/sys_arch.c
    # Vendored Arm LAN9118 (SMSC9220) low-level driver (Apache-2.0). The lwIP
    # netif wrapper over it (netif/EthernetIf.c) stays in the executable under
    # the strict bar; this third-party source sits under the relaxations.
    ${CMAKE_CURRENT_SOURCE_DIR}/netif/smsc9220/smsc9220_eth_drv.c
)

target_compile_options(solid_syslog_freertos_lwip_upstream PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -fno-common
    # FreeRTOS-Kernel + lwIP + the Arm driver use non-prototype declarations,
    # type-narrowing constructs, sign-conversion patterns, and shadowed locals
    # that trip our strict host warnings. Scoped to this OBJECT library so the
    # relaxations never reach project code.
    -Wno-unused-parameter
    -Wno-unused-but-set-variable
    -Wno-implicit-fallthrough
    -Wno-array-bounds
    -Wno-conversion
    -Wno-sign-conversion
    -Wno-shadow
    -Wno-pedantic
)

target_include_directories(solid_syslog_freertos_lwip_upstream PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                       # FreeRTOSConfig.h, lwipopts.h, arch/cc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/netif/smsc9220        # smsc9220_eth_drv.h, smsc9220_emac_config.h
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_PORT_DIR}                              # portmacro.h
    ${LWIP_DIR}/src/include
    ${LWIP_CONTRIB_FREERTOS_DIR}/include              # arch/sys_arch.h
)

# --- Executable ---------------------------------------------------------------
#
# Project code only. Inherits the full project warning set from the top-level
# CMakeLists.txt — no -Wno-* relaxations here.

add_executable(SolidSyslogBddTargetLwip
    main.c
    ${FREERTOS_TCP_TARGET_DIR}/Startup.c              # Ethernet variant: IRQ 13 -> EthernetISR
    ${FREERTOS_COMMON_DIR}/CmsdkUart.c
    ${FREERTOS_COMMON_DIR}/Syscalls.c
    netif/EthernetIf.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawAddress.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawAddressMessages.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawAddressStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawResolver.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawResolverMessages.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawResolverStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawDatagram.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawDatagramMessages.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawDatagramStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawTcpStream.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawTcpStreamMessages.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawTcpStreamStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source/SolidSyslogLwipRawMarshal.c
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosMutex.c
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosMutexMessages.c
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosMutexStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosSysUpTime.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetInteractive.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetIps.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetLanguage.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetSwitchConfig.c
    $<TARGET_OBJECTS:solid_syslog_freertos_lwip_upstream>
)

set_target_properties(SolidSyslogBddTargetLwip PROPERTIES SUFFIX ".elf")

target_compile_options(SolidSyslogBddTargetLwip PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -fno-common
)

target_include_directories(SolidSyslogBddTargetLwip PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                       # FreeRTOSConfig.h, lwipopts.h, arch/cc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/netif                 # EthernetIf.h
    ${CMAKE_CURRENT_SOURCE_DIR}/netif/smsc9220        # smsc9220_eth_drv.h, smsc9220_emac_config.h
    ${FREERTOS_COMMON_DIR}                            # CmsdkUart.h
    ${CMAKE_SOURCE_DIR}/Core/Interface                # SolidSyslog*.h
    ${CMAKE_SOURCE_DIR}/Core/Source                   # internal headers consumed by adapters
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Interface    # SolidSyslogLwipRaw{Address,Resolver,Datagram,TcpStream,Marshal}.h
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source        # SolidSyslogLwipRaw*Private.h
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Interface   # SolidSyslogFreeRtos{Mutex,SysUpTime}.h
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source      # SolidSyslogFreeRtosMutexPrivate.h
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common            # BddTargetInteractive.h, BddTargetSwitchConfig.h, …
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_PORT_DIR}                              # portmacro.h
    ${LWIP_DIR}/src/include
    ${LWIP_CONTRIB_FREERTOS_DIR}/include              # arch/sys_arch.h (via lwip/sys.h)
)

target_link_libraries(SolidSyslogBddTargetLwip PRIVATE
    ${PROJECT_NAME}                                   # libSolidSyslog.a (cross-compiled)
)

target_link_options(SolidSyslogBddTargetLwip PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -T${FREERTOS_COMMON_DIR}/mps2-an385.ld
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE:SolidSyslogBddTargetLwip>.map
)

set_target_properties(SolidSyslogBddTargetLwip PROPERTIES
    LINK_DEPENDS ${FREERTOS_COMMON_DIR}/mps2-an385.ld)
