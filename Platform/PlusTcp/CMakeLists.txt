# FreeRTOS-Plus-TCP adapter pack — shipped as sources, not a precompiled library.
#
# FreeRTOS and Mbed TLS are header-configured: the integrator's
# FreeRTOSConfig.h / FreeRTOSIPConfig.h flows through upstream headers and
# changes the size/layout of TCBs, sockets, and IP-task internals. Compiling
# our adapters once into libSolidSyslog.a would silently break consumers
# that use a different config.
#
# Instead, this is an INTERFACE library — each consumer (Bdd/Targets/FreeRtos,
# Tests/FreeRtos/*Test, downstream integrator apps) recompiles the adapter
# sources with its own config on the include path. Mirrors Platform/FreeRtos/
# (kernel primitives — Mutex, SysUpTime) and Platform/MbedTls/ — see
# project_header_configured_platforms memory.
#
# The pack CARRIES its umbrella sources via target_sources(INTERFACE) (S30.02):
# a consumer that links SolidSyslog::PlusTcp compiles the adapter .c files into
# its own target, seeing its own FreeRTOSConfig.h / FreeRTOSIPConfig.h — no
# per-file listing. The consumer still points at its own FreeRTOS-Plus-TCP tree.

add_library(SolidSyslogPlusTcp INTERFACE)

# Umbrella = all four FreeRTOS-Plus-TCP networking adapters (no config-gated
# optional): Address, Datagram, Resolver, TcpStream.
target_sources(SolidSyslogPlusTcp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogPlusTcpAddress.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogPlusTcpAddressStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogPlusTcpDatagram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogPlusTcpDatagramStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogPlusTcpResolver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogPlusTcpResolverStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogPlusTcpTcpStream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogPlusTcpTcpStreamStatic.c
)

target_include_directories(SolidSyslogPlusTcp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogPlusTcp*Private.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogPlusTcp INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::PlusTcp ALIAS SolidSyslogPlusTcp)

solidsyslog_install_public_headers()
