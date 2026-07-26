# lwIP Raw API adapter pack — shipped as sources, not a precompiled library.
#
# lwIP is header-configured (lwipopts.h): LWIP_RAW / LWIP_UDP / LWIP_TCP /
# NO_SYS / MEM_SIZE / PBUF pool sizing all change the types and behaviour
# the wrapper interacts with. Compiling our adapters once into libSolidSyslog.a
# would silently break consumers that use a different lwipopts.h.
#
# Instead, this is an INTERFACE library — each consumer (Tests/SolidSyslogLwipRaw*Test,
# downstream integrator apps, and in S28.06 the FreeRtosLwip BDD target) recompiles
# the adapter sources with its own config on the include path. Mirrors Platform/PlusTcp/,
# Platform/FreeRtos/, and Platform/MbedTls/ — see project_header_configured_platforms
# memory.
#
# OS-agnostic by construction: zero direct calls to FreeRTOS / POSIX / Win32 / Zephyr
# primitives in Source/. Any host primitive needed (e.g. a wait primitive for DNS in
# the future S28.07 sibling) is abstracted behind a SolidSyslog* vtable, mirroring
# how SolidSyslogMutex abstracts locking. See E28 (#439) Architecture boundary.
#
# The pack CARRIES its umbrella sources via target_sources(INTERFACE) (S30.02):
# a consumer that links SolidSyslog::LwipRaw compiles the adapter .c files into
# its own target, seeing its own lwipopts.h — no per-file listing. The pack
# exports the SolidSyslog-side include dirs its sources need (its own
# Interface/Source, Core/Interface via SolidSyslogTunables, and Core/Source for
# the internal pool-allocator helper); the consumer still points at its own lwIP
# tree (lwip/*.h are consumer-located).

add_library(SolidSyslogLwipRaw INTERFACE)

# Umbrella = the adapters that compile in ANY valid lwIP config: Address,
# Datagram, TcpStream, the NUMERIC Resolver, and the Marshal hop. The DNS
# resolver is config-gated (needs LWIP_DNS=1) and is kept OUT of the umbrella —
# see the SolidSyslogLwipRawDnsResolver component below.
target_sources(SolidSyslogLwipRaw INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawAddress.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawAddressStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawDatagram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawDatagramStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawTcpStream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawTcpStreamStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawResolver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawResolverStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawMarshal.c
)

target_include_directories(SolidSyslogLwipRaw INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogLwipRaw*Private.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogLwipRaw INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::LwipRaw ALIAS SolidSyslogLwipRaw)

# Opt-in DNS resolver component — REQUIRES LWIP_DNS=1 in the consumer's
# lwipopts.h. Kept out of the umbrella so a numeric-only build never has to
# enable DNS. Links the umbrella, so a consumer that wants DNS links just this
# one target and gets Address/Datagram/TcpStream/Marshal too.
add_library(SolidSyslogLwipRawDnsResolver INTERFACE)

target_sources(SolidSyslogLwipRawDnsResolver INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawDnsResolver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLwipRawDnsResolverStatic.c
)

target_link_libraries(SolidSyslogLwipRawDnsResolver INTERFACE SolidSyslogLwipRaw)

add_library(SolidSyslog::LwipRawDnsResolver ALIAS SolidSyslogLwipRawDnsResolver)

solidsyslog_install_public_headers()
