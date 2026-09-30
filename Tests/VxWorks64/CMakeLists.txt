# Tests/VxWorks64 - host-side unit tests for the VxWorks 6.4 adapters under
# Platform/VxWorks64/Source/. Each test executable recompiles the adapter source
# under test against Tests/Support/VxWorks64Fakes and links against the fakes.
#
# No upstream guard: the fakes supply the VxWorks headers themselves, so there
# is no tree to be missing.

add_executable(SolidSyslogVxWorks64MutexTest
    SolidSyslogVxWorks64MutexTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64MutexStatic.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source/VxWorks64SemFake.c
)

target_link_libraries(SolidSyslogVxWorks64MutexTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogVxWorks64MutexTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests/Support
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Interface
)

add_test(NAME SolidSyslogVxWorks64MutexTest COMMAND SolidSyslogVxWorks64MutexTest)
