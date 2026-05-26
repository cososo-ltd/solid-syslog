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

add_library(SolidSyslogLwipRaw INTERFACE)

target_include_directories(SolidSyslogLwipRaw INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
