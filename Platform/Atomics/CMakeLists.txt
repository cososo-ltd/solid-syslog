if(HAVE_STDATOMIC_H)
    target_sources(${PROJECT_NAME} PRIVATE
        Source/SolidSyslogStdAtomicCounter.c
        Source/SolidSyslogStdAtomicCounterMessages.c
        Source/SolidSyslogStdAtomicCounterStatic.c
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Interface)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Interface/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
