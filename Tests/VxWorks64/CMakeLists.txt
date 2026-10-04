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

# The counter's own tests beside the shared AtomicCounter contract suite. The
# helper whitebox-includes the counter's source, so only its Static half is
# listed here.
add_executable(SolidSyslogVxWorks64AtomicCounterTest
    ${CMAKE_SOURCE_DIR}/Tests/SolidSyslogAtomicCounterContractTest.cpp
    SolidSyslogVxWorks64AtomicCounterTest.cpp
    SolidSyslogVxWorks64AtomicCounterPoolTest.cpp
    SolidSyslogVxWorks64AtomicCounterTestHelper.c
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64AtomicCounterStatic.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source/VxWorks64IntFake.c
)

target_link_libraries(SolidSyslogVxWorks64AtomicCounterTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogVxWorks64AtomicCounterTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests
    ${CMAKE_SOURCE_DIR}/Tests/Support
)

set_target_properties(SolidSyslogVxWorks64AtomicCounterTest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

add_test(NAME SolidSyslogVxWorks64AtomicCounterTest COMMAND SolidSyslogVxWorks64AtomicCounterTest)

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

add_executable(SolidSyslogVxWorks64DatagramTest
    SolidSyslogVxWorks64DatagramTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Datagram.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64DatagramStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Address.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64AddressStatic.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source/VxWorks64NetFake.c
)

target_link_libraries(SolidSyslogVxWorks64DatagramTest PRIVATE
    ${PROJECT_NAME}
    ConfigLockFake
    ErrorHandlerFake
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogVxWorks64DatagramTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests/Support
)

set_target_properties(SolidSyslogVxWorks64DatagramTest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

add_test(NAME SolidSyslogVxWorks64DatagramTest COMMAND SolidSyslogVxWorks64DatagramTest)

# The clock's stand-in time.h shadows the host's, so it is applied only to the C
# sources that call the VxWorks time API; the C++ test drives the fake through
# VxWorks64ClockFake.h and never sees it.
set(VXWORKS64_CLOCK_SOURCES
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Clock.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source/VxWorks64ClockFake.c
)

set_source_files_properties(${VXWORKS64_CLOCK_SOURCES} ${CMAKE_SOURCE_DIR}/Bdd/Targets/VxWorks64/BddTargetVxWorks64Clock.c PROPERTIES
    INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/TimeInterface
)

add_executable(SolidSyslogVxWorks64ClockTest
    SolidSyslogVxWorks64ClockTest.cpp
    main.cpp
    ${VXWORKS64_CLOCK_SOURCES}
)

target_link_libraries(SolidSyslogVxWorks64ClockTest PRIVATE
    ${PROJECT_NAME}
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogVxWorks64ClockTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Tests/Support
)

set_target_properties(SolidSyslogVxWorks64ClockTest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

add_test(NAME SolidSyslogVxWorks64ClockTest COMMAND SolidSyslogVxWorks64ClockTest)

add_executable(SolidSyslogVxWorks64SysUpTimeTest
    SolidSyslogVxWorks64SysUpTimeTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64SysUpTime.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source/VxWorks64TaskFake.c
)

target_link_libraries(SolidSyslogVxWorks64SysUpTimeTest PRIVATE
    ${PROJECT_NAME}
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogVxWorks64SysUpTimeTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Tests/Support
)

set_target_properties(SolidSyslogVxWorks64SysUpTimeTest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

add_test(NAME SolidSyslogVxWorks64SysUpTimeTest COMMAND SolidSyslogVxWorks64SysUpTimeTest)

add_executable(SolidSyslogVxWorks64HostnameTest
    SolidSyslogVxWorks64HostnameTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Hostname.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source/VxWorks64NetFake.c
)

target_link_libraries(SolidSyslogVxWorks64HostnameTest PRIVATE
    ${PROJECT_NAME}
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogVxWorks64HostnameTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests/Support
)

set_target_properties(SolidSyslogVxWorks64HostnameTest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

add_test(NAME SolidSyslogVxWorks64HostnameTest COMMAND SolidSyslogVxWorks64HostnameTest)

add_executable(SolidSyslogVxWorks64SleepTest
    SolidSyslogVxWorks64SleepTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Sleep.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source/VxWorks64TaskFake.c
)

target_link_libraries(SolidSyslogVxWorks64SleepTest PRIVATE
    ${PROJECT_NAME}
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogVxWorks64SleepTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Tests/Support
)

set_target_properties(SolidSyslogVxWorks64SleepTest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

add_test(NAME SolidSyslogVxWorks64SleepTest COMMAND SolidSyslogVxWorks64SleepTest)

# The BDD target compiles here against the same fakes, so a change that breaks
# it fails on the host as well as in the target build.
add_executable(BddTargetVxWorks64Test
    BddTargetVxWorks64Test.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/VxWorks64/BddTargetVxWorks64.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/VxWorks64/BddTargetVxWorks64Clock.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetCustomSd.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetErrorText.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetInteractive.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetIps.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetLanguage.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetMessageSettings.c
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetServiceThread.c
    ${VXWORKS64_CLOCK_SOURCES}
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Hostname.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Address.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64AddressStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64AtomicCounter.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64AtomicCounterStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Datagram.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64DatagramStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Mutex.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64MutexStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Resolver.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64ResolverStatic.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64Sleep.c
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source/SolidSyslogVxWorks64SysUpTime.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source/VxWorks64IntFake.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source/VxWorks64NetFake.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source/VxWorks64SemFake.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source/VxWorks64TaskFake.c
)

target_link_libraries(BddTargetVxWorks64Test PRIVATE
    ${PROJECT_NAME}
    CppUTest
    CppUTestExt
)

target_include_directories(BddTargetVxWorks64Test PRIVATE
    ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Interface
    ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/VxWorks64
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests/Support
)

set_target_properties(BddTargetVxWorks64Test PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

# The test feeds the console through a temporary file, whose portable spelling
# CMake picks as it does SafeString's.
if(MSVC)
    target_sources(BddTargetVxWorks64Test PRIVATE ${CMAKE_SOURCE_DIR}/Tests/Support/TempFileWindows.c)
else()
    target_sources(BddTargetVxWorks64Test PRIVATE ${CMAKE_SOURCE_DIR}/Tests/Support/TempFileStandard.c)
endif()

add_test(NAME BddTargetVxWorks64Test COMMAND BddTargetVxWorks64Test)
