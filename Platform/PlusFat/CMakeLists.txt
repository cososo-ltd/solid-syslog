# FreeRTOS-Plus-FAT adapter pack. An INTERFACE library rather than a precompiled
# one: FreeRTOSFATConfig.h changes the layout of FF_FILE and FF_IOManager, and
# Plus-FAT is FreeRTOS-coupled (ff_headers.h pulls FreeRTOS.h / task.h /
# semphr.h), so each consumer compiles these sources against their own config.
# Linking SolidSyslog::PlusFat is enough — the pack carries its sources. The
# consumer points at their own Plus-FAT tree and supplies their FF_Disk_t driver.

add_library(SolidSyslogPlusFat INTERFACE)

target_sources(SolidSyslogPlusFat INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogPlusFatFile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogPlusFatFileStatic.c
)

target_include_directories(SolidSyslogPlusFat INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogPlusFatFilePrivate.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogPlusFat INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::PlusFat ALIAS SolidSyslogPlusFat)

solidsyslog_install_public_headers()
