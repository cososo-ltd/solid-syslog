if(SOLIDSYSLOG_SPINORFLASH)
    target_sources(${PROJECT_NAME} PRIVATE
        Source/SolidSyslogSpiNorFlash.c
        Source/SolidSyslogSpiNorFlashErrorsName.c
        Source/SolidSyslogSpiNorFlashSfdp.c
        Source/SolidSyslogSpiNorFlashStatic.c
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Interface)

solidsyslog_install_public_headers()
