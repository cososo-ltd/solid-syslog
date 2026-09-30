# Tests/VxWorks64 - host-side unit tests for the VxWorks 6.4 adapters under
# Platform/VxWorks64/Source/. Each test executable recompiles the adapter source
# under test against Tests/Support/VxWorks64Fakes and links against the fakes.
#
# No upstream guard: the fakes supply the VxWorks headers themselves, so there
# is no tree to be missing.

add_executable(SolidSyslogVxWorks64MutexTest
    SolidSyslogVxWorks64MutexTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Mutex.c
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

# The pack's sources, and the fakes standing in for the VxWorks headers, compile
# here at strict C99: the toolchains VxWorks 6.4 ships predate C11, so this is
# the host check that nothing newer creeps in.
set_target_properties(SolidSyslogVxWorks64MutexTest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

add_executable(SolidSyslogVxWorks64AddressTest
    SolidSyslogVxWorks64AddressTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Address.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64AddressStatic.c
)

target_link_libraries(SolidSyslogVxWorks64AddressTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogVxWorks64AddressTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests/Support
)

set_target_properties(SolidSyslogVxWorks64AddressTest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

add_test(NAME SolidSyslogVxWorks64AddressTest COMMAND SolidSyslogVxWorks64AddressTest)

add_executable(SolidSyslogVxWorks64ResolverTest
    SolidSyslogVxWorks64ResolverTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Resolver.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64ResolverStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Address.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64AddressStatic.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source/VxWorks64NetFake.c
)

target_link_libraries(SolidSyslogVxWorks64ResolverTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogVxWorks64ResolverTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests/Support
)

set_target_properties(SolidSyslogVxWorks64ResolverTest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

add_test(NAME SolidSyslogVxWorks64ResolverTest COMMAND SolidSyslogVxWorks64ResolverTest)
