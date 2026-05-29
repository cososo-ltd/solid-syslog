# FreeRTOS + lwIP (Raw API) link-probe ELF for QEMU mps2-an385 (Cortex-M3).
#
# S28.07: cross-build the Platform/LwipRaw adapter tree for a FreeRTOS/ARM
# target, selected via SOLIDSYSLOG_FREERTOS_NET=LWIP, proving it links against
# lwIP core with zero dependency on Platform/PlusTcp / FreeRTOS-Plus-TCP. No
# LAN9118 netif driver and no QEMU run — those land with S28.09, which reuses
# this directory's lwipopts.h / FreeRTOSConfig.h / arch/cc.h and grows main.c
# into the real netif + tcpip integration.
#
# Source layout mirrors Bdd/Targets/FreeRtos/CMakeLists.txt: upstream
# FreeRTOS-Kernel + lwIP-core sources compile into a separate OBJECT library
# (`solid_syslog_freertos_lwip_upstream`) under a relaxed warning set; our own
# code (main.c, the reused Cortex-M3 startup, and the Platform/LwipRaw adapter
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

# lwIP ships its source lists as *_SRCS variables via Filelists.cmake (it is
# explicitly NOT an add_subdirectory target). We consume the minimal IPv4 core
# set — no api/ (NO_SYS=1, no sequential/socket API), no ipv6, no netif driver.
include(${LWIP_DIR}/src/Filelists.cmake)

set(FREERTOS_PORT_DIR "${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM3")
set(FREERTOS_COMMON_DIR "${CMAKE_SOURCE_DIR}/Bdd/Targets/FreeRtos/Common")

# --- Upstream OBJECT library --------------------------------------------------
#
# FreeRTOS kernel (minimal: scheduler + heap) + lwIP IPv4 core, compiled with
# the relaxed warning set those upstream sources require. The executable
# consumes the objects via $<TARGET_OBJECTS:...> so the relaxations don't leak.

add_library(solid_syslog_freertos_lwip_upstream OBJECT
    ${FREERTOS_KERNEL_PATH}/tasks.c
    ${FREERTOS_KERNEL_PATH}/queue.c
    ${FREERTOS_KERNEL_PATH}/list.c
    ${FREERTOS_PORT_DIR}/port.c
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
    ${lwipcore_SRCS}
    ${lwipcore4_SRCS}
    # etharp (in core4) emits ARP frames via ethernet_output/ethbroadcast/
    # ethzero from netif/ethernet.c — pull that one netif source in (but not
    # bridgeif/slipif from lwipnetif_SRCS, which this probe has no use for).
    ${LWIP_DIR}/src/netif/ethernet.c
)

target_compile_options(solid_syslog_freertos_lwip_upstream PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -fno-common
    # FreeRTOS-Kernel + lwIP use non-prototype declarations, type-narrowing
    # constructs, sign-conversion patterns, and shadowed locals that trip our
    # strict host warnings. Scoped to this OBJECT library so the relaxations
    # never reach project code.
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
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_PORT_DIR}                              # portmacro.h
    ${LWIP_DIR}/src/include
)

# --- Executable ---------------------------------------------------------------
#
# Project code only. Inherits the full project warning set from the top-level
# CMakeLists.txt — no -Wno-* relaxations here.

add_executable(SolidSyslogBddTargetLwip
    main.c
    ${FREERTOS_COMMON_DIR}/startup.c
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
    ${CMAKE_SOURCE_DIR}/Core/Interface                # SolidSyslog*.h
    ${CMAKE_SOURCE_DIR}/Core/Source                   # internal headers consumed by adapters
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Interface    # SolidSyslogLwipRaw{Address,Resolver,Datagram,TcpStream}.h
    ${CMAKE_SOURCE_DIR}/Platform/LwipRaw/Source        # SolidSyslogLwipRaw*Private.h
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_PORT_DIR}                              # portmacro.h
    ${LWIP_DIR}/src/include
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
