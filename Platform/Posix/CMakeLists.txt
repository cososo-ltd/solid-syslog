target_sources(${PROJECT_NAME} PRIVATE
    Source/SolidSyslogPosixClock.c
    Source/SolidSyslogPosixHostname.c
    Source/SolidSyslogPosixMessageQueueBuffer.c
    Source/SolidSyslogPosixMessageQueueBufferStatic.c
    Source/SolidSyslogPosixMutex.c
    Source/SolidSyslogPosixMutexStatic.c
    Source/SolidSyslogPosixProcessId.c
    Source/SolidSyslogPosixSysUpTime.c
    Source/SolidSyslogPosixSleep.c
    Source/SolidSyslogAddress.c
    Source/SolidSyslogGetAddrInfoResolver.c
    Source/SolidSyslogGetAddrInfoResolverStatic.c
    Source/SolidSyslogPosixDatagram.c
    Source/SolidSyslogPosixDatagramStatic.c
    Source/SolidSyslogPosixTcpStream.c
    Source/SolidSyslogPosixTcpStreamStatic.c
    Source/SolidSyslogPosixFile.c
    Source/SolidSyslogPosixFileStatic.c
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Interface)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
