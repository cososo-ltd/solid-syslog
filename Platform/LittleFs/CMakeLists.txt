# LittleFS adapter pack. An INTERFACE library: lfs.h comes from the consumer's
# own LittleFS checkout rather than from us, so these sources compile in the
# CONSUMER's target against the configuration their build sets. Linking
# SolidSyslog::LittleFs is enough - the pack carries its sources.

add_library(SolidSyslogLittleFs INTERFACE)

target_sources(SolidSyslogLittleFs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLittleFsFile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SolidSyslogLittleFsFileStatic.c
)

target_include_directories(SolidSyslogLittleFs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/Source          # SolidSyslogLittleFsFilePrivate.h
    ${SolidSyslog_SOURCE_DIR}/Core/Source             # SolidSyslogPoolAllocator.h
)

target_link_libraries(SolidSyslogLittleFs INTERFACE SolidSyslogTunables)

add_library(SolidSyslog::LittleFs ALIAS SolidSyslogLittleFs)

solidsyslog_install_public_headers()
