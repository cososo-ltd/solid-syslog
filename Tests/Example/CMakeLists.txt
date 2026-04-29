# Platform-agnostic tests always included.
set(EXAMPLE_TEST_SOURCES
    ExampleAppNameTest.cpp
    ExampleLanguageTest.cpp
    main.cpp
)

set(EXAMPLE_PRODUCTION_SOURCES
    ${CMAKE_SOURCE_DIR}/Example/Common/ExampleAppName.c
    ${CMAKE_SOURCE_DIR}/Example/Common/ExampleLanguage.c
)

set(EXAMPLE_TEST_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/Example/Common
)

if(SOLIDSYSLOG_POSIX)
    list(APPEND EXAMPLE_TEST_SOURCES
        ExampleCommandLineTest.cpp
        SolidSyslogExampleTest.cpp
        ExampleServiceThreadTest.cpp
        ExampleSwitchConfigTest.cpp
    )
    list(APPEND EXAMPLE_PRODUCTION_SOURCES
        ${CMAKE_SOURCE_DIR}/Example/SingleTask/SolidSyslogExample.c
        ${CMAKE_SOURCE_DIR}/Example/Common/ExampleCommandLine.c
        ${CMAKE_SOURCE_DIR}/Example/Common/ExampleServiceThread.c
        ${CMAKE_SOURCE_DIR}/Example/Common/ExampleTcpConfig.c
        ${CMAKE_SOURCE_DIR}/Example/Common/ExampleUdpConfig.c
        ${CMAKE_SOURCE_DIR}/Example/Common/ExampleInteractive.c
        ${CMAKE_SOURCE_DIR}/Example/Common/ExampleSwitchConfig.c
    )
    list(APPEND EXAMPLE_TEST_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/Example/SingleTask
    )
endif()

if(SOLIDSYSLOG_WINSOCK AND HAVE_WINDOWS_PLATFORM)
    list(APPEND EXAMPLE_TEST_SOURCES
        ExampleWindowsCommandLineTest.cpp
    )
    list(APPEND EXAMPLE_PRODUCTION_SOURCES
        ${CMAKE_SOURCE_DIR}/Example/Windows/ExampleWindowsCommandLine.c
    )
    list(APPEND EXAMPLE_TEST_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/Example/Windows
    )
endif()

add_executable(ExampleTests ${EXAMPLE_TEST_SOURCES})

target_link_libraries(ExampleTests PRIVATE ${PROJECT_NAME} CppUTest CppUTestExt Threads::Threads)

if(SOLIDSYSLOG_POSIX)
    target_link_libraries(ExampleTests PRIVATE PosixFakes)
endif()

target_include_directories(ExampleTests PRIVATE ${EXAMPLE_TEST_INCLUDE_DIRS})

target_sources(ExampleTests PRIVATE ${EXAMPLE_PRODUCTION_SOURCES})
