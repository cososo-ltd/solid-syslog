# lwIP Sockets API adapter pack. An INTERFACE library rather than a precompiled
# one: lwipopts.h (LWIP_SOCKET / LWIP_DNS / LWIP_TCP / LWIP_UDP) changes the types
# and behaviour these wrappers interact with, so each consumer compiles them
# against their own config. Linking SolidSyslog::LwipSocket is enough - the pack
# carries its sources and exports the SolidSyslog-side include dirs they need. The
# consumer points at their own lwIP tree.
#
# OS-agnostic by construction: no direct RTOS or host OS calls in Source/. The
# sockets API is thread-safe and needs no marshal, which is what this pack trades
# a larger upstream footprint for.

add_library(SolidSyslogLwipSocket INTERFACE)

# Config-gated adapters are in the umbrella: each compiles to nothing when the
# lwIP option it needs is off.
target_sources(SolidSyslogLwipSocket INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipSocketAddress.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipSocketAddressStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipSocketDatagram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipSocketDatagramStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipSocketResolver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipSocketResolverStatic.c
)

target_include_directories(SolidSyslogLwipSocket INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogLwipSocket*Private.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source       # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogLwipSocket INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::LwipSocket ALIAS SolidSyslogLwipSocket)

solidsyslog_install_public_headers()
