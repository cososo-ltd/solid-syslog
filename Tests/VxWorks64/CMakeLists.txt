# Tests/VxWorks64 - host-side unit tests for the VxWorks 6.4 adapters under
# Platform/VxWorks64/Source/. Each test executable recompiles the adapter source
# under test against Tests/Support/VxWorks64Fakes and links against the fakes.
#
# No upstream guard: the fakes supply the VxWorks headers themselves, so there
# is no tree to be missing.

set(VXWORKS64_SOURCE ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Source)
set(VXWORKS64_FAKES ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Source)
set(VXWORKS64_SHADOW_INTERFACE ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/ShadowInterface)

# Defines a VxWorks 6.4 test executable whole, so none can be defined without
# running: main.cpp, the standard links and includes, add_test, and junit
# registration, which Tests/CMakeLists.txt completes once the junit target exists.
#
#   SOURCES    the test's own sources, the adapters under test and their fakes
#   SHADOWED   C sources that call a VxWorks API whose header shares a host
#              header's name; only these see the stand-ins in
#              Tests/Support/VxWorks64Fakes/ShadowInterface, never a C++ test
#   LIBRARIES  links beyond the project library and CppUTest
#   INCLUDES   include directories beyond the standard set
#
# The pack's sources, and the fakes standing in for the VxWorks headers, compile
# at strict C99: the toolchains VxWorks 6.4 ships predate C11, so this is the
# host check that nothing newer creeps in.
function(vxworks64_add_test name)
    cmake_parse_arguments(PARSE_ARGV 1 TEST "" "" "SOURCES;SHADOWED;LIBRARIES;INCLUDES")

    add_executable(${name}
        ${TEST_SOURCES}
        ${TEST_SHADOWED}
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    )

    if(TEST_SHADOWED)
        set_source_files_properties(${TEST_SHADOWED} PROPERTIES
            INCLUDE_DIRECTORIES ${VXWORKS64_SHADOW_INTERFACE}
        )
    endif()

    target_link_libraries(${name} PRIVATE
        ${PROJECT_NAME}
        ${TEST_LIBRARIES}
        CppUTest
        CppUTestExt
    )

    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/Tests/Support/VxWorks64Fakes/Interface
        ${CMAKE_SOURCE_DIR}/Platform/VxWorks64/Interface
        ${VXWORKS64_SOURCE}
        ${CMAKE_SOURCE_DIR}/Core/Interface
        ${CMAKE_SOURCE_DIR}/Core/Source
        ${CMAKE_SOURCE_DIR}/Tests/Support
        ${TEST_INCLUDES}
    )

    set_target_properties(${name} PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
    )

    add_test(NAME ${name} COMMAND ${name})
    set_property(GLOBAL APPEND PROPERTY SOLIDSYSLOG_VXWORKS64_JUNIT_TESTS ${name})
endfunction()

vxworks64_add_test(SolidSyslogVxWorks64MutexTest
    SOURCES
        SolidSyslogVxWorks64MutexTest.cpp
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Mutex.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64MutexStatic.c
        ${VXWORKS64_FAKES}/VxWorks64SemFake.c
    LIBRARIES
        ConfigLockFake
        ErrorHandlerFake
)

# The counter's own tests beside the shared AtomicCounter contract suite. The
# helper whitebox-includes the counter's source, so only its Static half is
# listed here.
vxworks64_add_test(SolidSyslogVxWorks64AtomicCounterTest
    SOURCES
        ${CMAKE_SOURCE_DIR}/Tests/SolidSyslogAtomicCounterContractTest.cpp
        SolidSyslogVxWorks64AtomicCounterTest.cpp
        SolidSyslogVxWorks64AtomicCounterPoolTest.cpp
        SolidSyslogVxWorks64AtomicCounterTestHelper.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64AtomicCounterStatic.c
        ${VXWORKS64_FAKES}/VxWorks64IntFake.c
    LIBRARIES
        ConfigLockFake
        ErrorHandlerFake
    INCLUDES
        ${CMAKE_SOURCE_DIR}/Tests
)

vxworks64_add_test(SolidSyslogVxWorks64AddressTest
    SOURCES
        SolidSyslogVxWorks64AddressTest.cpp
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Address.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64AddressStatic.c
    LIBRARIES
        ConfigLockFake
        ErrorHandlerFake
)

vxworks64_add_test(SolidSyslogVxWorks64ResolverTest
    SOURCES
        SolidSyslogVxWorks64ResolverTest.cpp
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Resolver.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64ResolverStatic.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Address.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64AddressStatic.c
        ${VXWORKS64_FAKES}/VxWorks64NetFake.c
    LIBRARIES
        ConfigLockFake
        ErrorHandlerFake
)

vxworks64_add_test(SolidSyslogVxWorks64DatagramTest
    SOURCES
        SolidSyslogVxWorks64DatagramTest.cpp
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Datagram.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64DatagramStatic.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Address.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64AddressStatic.c
        ${VXWORKS64_FAKES}/VxWorks64IoFake.c
        ${VXWORKS64_FAKES}/VxWorks64NetFake.c
    LIBRARIES
        ConfigLockFake
        ErrorHandlerFake
)

vxworks64_add_test(SolidSyslogVxWorks64TcpStreamTest
    SOURCES
        SolidSyslogVxWorks64TcpStreamTest.cpp
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64TcpStream.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64TcpStreamStatic.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Address.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64AddressStatic.c
        ${VXWORKS64_FAKES}/VxWorks64IoFake.c
        ${VXWORKS64_FAKES}/VxWorks64NetFake.c
    LIBRARIES
        ConfigLockFake
        ErrorHandlerFake
)

# The C++ test drives the time fake through VxWorks64ClockFake.h, so only the
# clock and its fake are shadowed.
set(VXWORKS64_CLOCK_SOURCES
    ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Clock.c
    ${VXWORKS64_FAKES}/VxWorks64ClockFake.c
)

vxworks64_add_test(SolidSyslogVxWorks64ClockTest
    SOURCES
        SolidSyslogVxWorks64ClockTest.cpp
    SHADOWED
        ${VXWORKS64_CLOCK_SOURCES}
)

vxworks64_add_test(SolidSyslogVxWorks64SysUpTimeTest
    SOURCES
        SolidSyslogVxWorks64SysUpTimeTest.cpp
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64SysUpTime.c
        ${VXWORKS64_FAKES}/VxWorks64TaskFake.c
)

vxworks64_add_test(SolidSyslogVxWorks64HostnameTest
    SOURCES
        SolidSyslogVxWorks64HostnameTest.cpp
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Hostname.c
        ${VXWORKS64_FAKES}/VxWorks64NetFake.c
)

vxworks64_add_test(SolidSyslogVxWorks64SleepTest
    SOURCES
        SolidSyslogVxWorks64SleepTest.cpp
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Sleep.c
        ${VXWORKS64_FAKES}/VxWorks64TaskFake.c
)

# The test feeds the console through a temporary file, whose portable spelling
# CMake picks as it does SafeString's.
if(MSVC)
    set(BDD_TARGET_VXWORKS64_TEMP_FILE ${CMAKE_SOURCE_DIR}/Tests/Support/TempFileWindows.c)
else()
    set(BDD_TARGET_VXWORKS64_TEMP_FILE ${CMAKE_SOURCE_DIR}/Tests/Support/TempFileStandard.c)
endif()

vxworks64_add_test(SolidSyslogVxWorks64FileTest
    SOURCES
        SolidSyslogVxWorks64FileTest.cpp
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64File.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64FileStatic.c
        ${VXWORKS64_FAKES}/VxWorks64IoFake.c
    LIBRARIES
        ConfigLockFake
        ErrorHandlerFake
)

# The store's sys/stat.h stand-in shadows the host's, so only the store and its
# fake see it.
vxworks64_add_test(BddTargetVxWorks64StoreTest
    SOURCES
        BddTargetVxWorks64StoreTest.cpp
    SHADOWED
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/VxWorks64/BddTargetVxWorks64Store.c
        ${VXWORKS64_FAKES}/VxWorks64FsFake.c
    INCLUDES
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/VxWorks64
)

# The BDD target compiles here against the same fakes, so a change that breaks
# it fails on the host as well as in the target build.
vxworks64_add_test(BddTargetVxWorks64Test
    SOURCES
        BddTargetVxWorks64Test.cpp
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/VxWorks64/BddTargetVxWorks64.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetCustomSd.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetErrorText.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetInteractive.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetIps.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetLanguage.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetMessageSettings.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetServiceThread.c
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common/BddTargetSwitchConfig.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Hostname.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Address.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64AddressStatic.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64AtomicCounter.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64AtomicCounterStatic.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Datagram.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64DatagramStatic.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Mutex.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64MutexStatic.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Resolver.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64ResolverStatic.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64Sleep.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64SysUpTime.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64TcpStream.c
        ${VXWORKS64_SOURCE}/SolidSyslogVxWorks64TcpStreamStatic.c
        ${VXWORKS64_FAKES}/VxWorks64IntFake.c
        ${VXWORKS64_FAKES}/VxWorks64IoFake.c
        ${VXWORKS64_FAKES}/VxWorks64NetFake.c
        ${VXWORKS64_FAKES}/VxWorks64SemFake.c
        ${VXWORKS64_FAKES}/VxWorks64TaskFake.c
        ${BDD_TARGET_VXWORKS64_TEMP_FILE}
    SHADOWED
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/VxWorks64/BddTargetVxWorks64Clock.c
        ${VXWORKS64_CLOCK_SOURCES}
    INCLUDES
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/Common
        ${CMAKE_SOURCE_DIR}/Bdd/Targets/VxWorks64
)
