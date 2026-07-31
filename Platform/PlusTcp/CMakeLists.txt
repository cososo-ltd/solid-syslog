# FreeRTOS-Plus-TCP adapter pack. An INTERFACE library rather than a precompiled
# one: FreeRTOSConfig.h / FreeRTOSIPConfig.h change the layout of TCBs, sockets
# and IP-task internals, so each consumer compiles these sources against their
# own config. Linking SolidSyslog::PlusTcp is enough — the pack carries its
# sources. The consumer points at their own Plus-TCP tree.

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
