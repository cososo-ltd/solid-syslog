# lwIP Raw API adapter pack. An INTERFACE library rather than a precompiled one:
# lwipopts.h (LWIP_RAW / LWIP_UDP / LWIP_TCP / NO_SYS / pool sizing) changes the
# types and behaviour these wrappers interact with, so each consumer compiles
# them against their own config. Linking SolidSyslog::LwipRaw is enough — the
# pack carries its sources and exports the SolidSyslog-side include dirs they
# need. The consumer points at their own lwIP tree.
#
# OS-agnostic by construction: no direct FreeRTOS / POSIX / Win32 / Zephyr calls
# in Source/. Any host primitive is reached through a SolidSyslog vtable, the way
# SolidSyslogMutex abstracts locking.

add_library(SolidSyslogLwipRaw INTERFACE)

# Config-gated adapters are in the umbrella: each compiles to nothing when the
# lwIP option it needs is off.
target_sources(SolidSyslogLwipRaw INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawAddress.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawAddressStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawDatagram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawDatagramStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawTcpStream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawTcpStreamStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawResolver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawResolverStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawDnsResolver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawDnsResolverStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawMarshal.c
)

target_include_directories(SolidSyslogLwipRaw INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogLwipRaw*Private.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogLwipRaw INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::LwipRaw ALIAS SolidSyslogLwipRaw)

solidsyslog_install_public_headers()
