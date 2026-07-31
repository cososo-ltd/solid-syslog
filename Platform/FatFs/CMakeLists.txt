# FatFs adapter pack. An INTERFACE library rather than a precompiled one: the
# integrator's ffconf.h changes the layout of FATFS and FIL and the visible API,
# so each consumer compiles these sources against their own config. Linking
# SolidSyslog::FatFs is enough — the pack carries its sources. The consumer
# points at their own FatFs tree and supplies their own diskio.c.

add_library(SolidSyslogFatFs INTERFACE)

target_sources(SolidSyslogFatFs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogFatFsFile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogFatFsFileStatic.c
)

target_include_directories(SolidSyslogFatFs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogFatFsFilePrivate.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogFatFs INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::FatFs ALIAS SolidSyslogFatFs)

solidsyslog_install_public_headers()
