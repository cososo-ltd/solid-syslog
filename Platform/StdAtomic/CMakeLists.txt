if(SOLIDSYSLOG_STDATOMIC)
    target_sources(${PROJECT_NAME} PRIVATE
        Source/SolidSyslogStdAtomicCounter.c
        Source/SolidSyslogStdAtomicCounterStatic.c
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Interface)

solidsyslog_install_public_headers()
