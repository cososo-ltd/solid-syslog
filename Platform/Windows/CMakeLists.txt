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

# Unconditional: the top level only enters this directory when SOLIDSYSLOG_WINDOWS
# is on, which already means at least one of the three groups above compiled.
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Interface)
solidsyslog_install_public_headers()

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
