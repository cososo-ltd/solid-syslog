# VxWorks 6.4 adapter pack. An INTERFACE library rather than a precompiled one:
# the VxWorks headers come from the consumer's Wind River installation rather
# than from us or from the system, so these sources compile in the CONSUMER's
# target against the headers their kernel ships. Linking SolidSyslog::VxWorks64
# is enough - the pack carries its sources.
#
# Our own tests need no Wind River installation: Tests/Support/VxWorks64Fakes
# supplies the API subset outright and the suite builds in the ordinary debug
# preset. That is why the registry row defaults it on rather than keying it to
# a path.

add_library(SolidSyslogVxWorks64 INTERFACE)

target_sources(SolidSyslogVxWorks64 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64Address.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64AddressStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64Mutex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64MutexStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64Resolver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64ResolverStatic.c
)

target_include_directories(SolidSyslogVxWorks64 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogVxWorks64MutexPrivate.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogVxWorks64 INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::VxWorks64 ALIAS SolidSyslogVxWorks64)

solidsyslog_install_public_headers()
