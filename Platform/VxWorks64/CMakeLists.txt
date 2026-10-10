# VxWorks 6.4 adapter pack. An INTERFACE library: the VxWorks headers come from
# the consumer's Wind River installation, so these sources compile in the
# consumer's target. Linking SolidSyslog::VxWorks64 brings the sources.
#
# The unit tests need no Wind River installation: Tests/Support/VxWorks64Fakes
# supplies the API subset, so the pack is on by default.

add_library(SolidSyslogVxWorks64 INTERFACE)

target_sources(SolidSyslogVxWorks64 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64Address.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64AddressStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64AtomicCounter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64AtomicCounterStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64Clock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64Datagram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64DatagramStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64File.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64FileStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64Hostname.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64Mutex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64MutexStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64Resolver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64ResolverStatic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64Sleep.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64SysUpTime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64TcpStream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogVxWorks64TcpStreamStatic.c
)

target_include_directories(SolidSyslogVxWorks64 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogVxWorks64MutexPrivate.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogVxWorks64 INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::VxWorks64 ALIAS SolidSyslogVxWorks64)

solidsyslog_install_public_headers()
