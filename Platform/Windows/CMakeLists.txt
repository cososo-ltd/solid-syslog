if(HAVE_WINDOWS_INTERLOCKED)
    target_sources(${PROJECT_NAME} PRIVATE
        Source/SolidSyslogWindowsAtomicCounter.c
        Source/SolidSyslogWindowsAtomicCounterStatic.c
    )
endif()

if(HAVE_WINDOWS_PLATFORM)
    target_sources(${PROJECT_NAME} PRIVATE
        Source/SolidSyslogWindowsClock.c
        Source/SolidSyslogWindowsFile.c
        Source/SolidSyslogWindowsFileStatic.c
        Source/SolidSyslogWindowsHostname.c
        Source/SolidSyslogWindowsMutex.c
        Source/SolidSyslogWindowsMutexStatic.c
        Source/SolidSyslogWindowsProcessId.c
        Source/SolidSyslogWindowsSleep.c
        Source/SolidSyslogWindowsSysUpTime.c
    )
endif()

if(HAVE_WINDOWS_INTERLOCKED OR HAVE_WINDOWS_PLATFORM OR SOLIDSYSLOG_WINSOCK)
    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Interface)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )
endif()

if(SOLIDSYSLOG_WINSOCK)
    target_sources(${PROJECT_NAME} PRIVATE
        Source/SolidSyslogWinsockAddress.c
        Source/SolidSyslogWinsockAddressStatic.c
        Source/SolidSyslogWinsockResolver.c
        Source/SolidSyslogWinsockResolverStatic.c
        Source/SolidSyslogWinsockDatagram.c
        Source/SolidSyslogWinsockDatagramStatic.c
        Source/SolidSyslogWinsockTcpStream.c
        Source/SolidSyslogWinsockTcpStreamStatic.c
    )
    target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32)
endif()
