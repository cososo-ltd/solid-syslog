# Tests/CmsisRtos — host-side unit tests for the CMSIS-RTOS2 adapters under
# Platform/CmsisRtos/Source/. Each test executable recompiles the adapter source
# under test against Tests/Support/CmsisRtosFakes and links against the fakes.
#
# No upstream guard here, unlike Tests/FreeRtos: the fakes supply cmsis_os2.h
# themselves, so there is no tree to be missing.

add_executable(SolidSyslogCmsisRtosMutexTest
    SolidSyslogCmsisRtosMutexTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/CmsisRtos/Source/SolidSyslogCmsisRtosMutex.c
    ${CMAKE_SOURCE_DIR}/Platform/CmsisRtos/Source/SolidSyslogCmsisRtosMutexStatic.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/CmsisRtosFakes/Source/CmsisRtosMutexFake.c
)

target_link_libraries(SolidSyslogCmsisRtosMutexTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogCmsisRtosMutexTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/CmsisRtos/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests/Support
    ${CMAKE_SOURCE_DIR}/Tests/Support/CmsisRtosFakes/Interface
)

add_test(NAME SolidSyslogCmsisRtosMutexTest COMMAND SolidSyslogCmsisRtosMutexTest)

add_executable(SolidSyslogCmsisRtosSysUpTimeTest
    SolidSyslogCmsisRtosSysUpTimeTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/CmsisRtos/Source/SolidSyslogCmsisRtosSysUpTime.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/CmsisRtosFakes/Source/CmsisRtosKernelFake.c
)

target_link_libraries(SolidSyslogCmsisRtosSysUpTimeTest PRIVATE
    ${PROJECT_NAME}
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogCmsisRtosSysUpTimeTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/CmsisRtos/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Tests/Support/CmsisRtosFakes/Interface
)

add_test(NAME SolidSyslogCmsisRtosSysUpTimeTest COMMAND SolidSyslogCmsisRtosSysUpTimeTest)
