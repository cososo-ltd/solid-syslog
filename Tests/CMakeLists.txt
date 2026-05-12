if(SOLIDSYSLOG_POSIX OR SOLIDSYSLOG_WINSOCK)
    add_subdirectory(Support)
endif()

if(SOLIDSYSLOG_POSIX OR (SOLIDSYSLOG_WINSOCK AND HAVE_WINDOWS_PLATFORM))
    add_subdirectory(Bdd/Targets)
endif()

if(DEFINED ENV{FREERTOS_KERNEL_PATH})
    add_subdirectory(Support/FreeRtosFakes)
    add_subdirectory(FreeRtos)
endif()

set(TEST_SOURCES
    SolidSyslogTest.cpp
    SolidSyslogTunablesTest.cpp
    SolidSyslogSwitchingSenderTest.cpp
    SolidSyslogMetaSdTest.cpp
    SolidSyslogTimeQualitySdTest.cpp
    SolidSyslogOriginSdTest.cpp
    SolidSyslogNullBufferTest.cpp
    SolidSyslogCircularBufferTest.cpp
    SolidSyslogNullMutexTest.cpp
    SolidSyslogNullStoreTest.cpp
    SolidSyslogNullSecurityPolicyTest.cpp
    SolidSyslogCrc16Test.cpp
    SolidSyslogCrc16PolicyTest.cpp
    SolidSyslogPrivalTest.cpp
    SolidSyslogErrorTest.cpp
    SolidSyslogFormatterTest.cpp
    SolidSyslogUdpPayloadTest.cpp
    BufferFakeTest.cpp
    BufferFake.c
    MutexFake.c
    StoreFakeTest.cpp
    StoreFake.c
    SenderFakeTest.cpp
    SenderFake.c
    DatagramFake.c
    DatagramFakeTest.cpp
    StringFakeTest.cpp
    StringFake.c
    FileFakeTest.cpp
    FileFake.c
    StreamFakeTest.cpp
    StreamFake.c
    TestAssert.cpp
    SolidSyslogBlockStoreTest.cpp
    SolidSyslogBlockDeviceTest.cpp
    SolidSyslogFileBlockDeviceTest.cpp
    BlockSequenceTest.cpp
    main.cpp
)

if(MSVC)
    list(APPEND TEST_SOURCES Support/SafeStringWindows.c)
else()
    list(APPEND TEST_SOURCES Support/SafeStringStandard.c)
endif()

if(HAVE_ATOMIC_COUNTER)
    list(APPEND TEST_SOURCES
        SolidSyslogAtomicCounterTest.cpp
        SolidSyslogAtomicU32Test.cpp
    )
endif()

if(SOLIDSYSLOG_POSIX)
    list(APPEND TEST_SOURCES
        SolidSyslogPosixClockTest.cpp
        SolidSyslogPosixMessageQueueBufferTest.cpp
        SolidSyslogPosixMutexTest.cpp
        SolidSyslogPosixSleepTest.cpp
        SolidSyslogPosixSysUpTimeTest.cpp
        ClockFakeTest.cpp
        SolidSyslogUdpSenderTest.cpp
        SolidSyslogStreamSenderTest.cpp
        SolidSyslogPosixFileTest.cpp
        SolidSyslogBlockStorePosixTest.cpp
        SolidSyslogGetAddrInfoResolverTest.cpp
        SolidSyslogPosixDatagramTest.cpp
        SolidSyslogPosixTcpStreamTest.cpp
        SocketFakeTest.cpp
    )
endif()

if(SOLIDSYSLOG_OPENSSL)
    list(APPEND TEST_SOURCES
        SolidSyslogTlsStreamTest.cpp
        OpenSslFakeTest.cpp
    )
endif()

if(SOLIDSYSLOG_POSIX OR SOLIDSYSLOG_WINSOCK)
    list(APPEND TEST_SOURCES SolidSyslogAddressTest.cpp)
endif()

if(HAVE_WINDOWS_PLATFORM)
    list(APPEND TEST_SOURCES
        SolidSyslogWindowsClockTest.cpp
        SolidSyslogWindowsFileTest.cpp
        SolidSyslogWindowsHostnameTest.cpp
        SolidSyslogWindowsMutexTest.cpp
        SolidSyslogWindowsProcessIdTest.cpp
        SolidSyslogWindowsSleepTest.cpp
        SolidSyslogWindowsSysUpTimeTest.cpp
    )
endif()

if(SOLIDSYSLOG_WINSOCK)
    list(APPEND TEST_SOURCES
        WinsockFakeTest.cpp
        SolidSyslogWinsockResolverTest.cpp
        SolidSyslogWinsockDatagramTest.cpp
        SolidSyslogWinsockTcpStreamTest.cpp
    )
endif()

add_executable(${PROJECT_NAME}Tests ${TEST_SOURCES})

target_link_libraries(${PROJECT_NAME}Tests PRIVATE ${PROJECT_NAME} ErrorHandlerFake CppUTest CppUTestExt Threads::Threads)
target_include_directories(${PROJECT_NAME}Tests PRIVATE ${CMAKE_SOURCE_DIR}/Core/Source ${CMAKE_CURRENT_SOURCE_DIR}/Support)

# Drives the static_assert in SolidSyslogTunablesTest.cpp when a tunable-override
# preset is active. Default builds leave it undefined; the assertion compiles out.
if(DEFINED SOLIDSYSLOG_TEST_EXPECTED_MAX_MESSAGE_SIZE)
    target_compile_definitions(${PROJECT_NAME}Tests PRIVATE
        SOLIDSYSLOG_TEST_EXPECTED_MAX_MESSAGE_SIZE=${SOLIDSYSLOG_TEST_EXPECTED_MAX_MESSAGE_SIZE})
endif()

if(SOLIDSYSLOG_POSIX)
    target_link_libraries(${PROJECT_NAME}Tests PRIVATE PosixFakes)
endif()

if(SOLIDSYSLOG_OPENSSL)
    target_link_libraries(${PROJECT_NAME}Tests PRIVATE OpenSslFakes)
endif()

if(HAVE_WINDOWS_PLATFORM OR SOLIDSYSLOG_WINSOCK)
    target_include_directories(${PROJECT_NAME}Tests PRIVATE ${CMAKE_SOURCE_DIR}/Platform/Windows/Source)
endif()

if(SOLIDSYSLOG_WINSOCK)
    target_link_libraries(${PROJECT_NAME}Tests PRIVATE WinsockFakes)
endif()

if(SOLIDSYSLOG_OPENSSL)
    add_subdirectory(OpenSslIntegration)
endif()

# Standard CppUTest console output via ctest -V
add_test(NAME ${PROJECT_NAME}Tests COMMAND ${PROJECT_NAME}Tests)

# JUnit XML output for CI and the VS Code test explorer.
# Uses a cmake script so test failures (non-zero exit) don't abort the target.
add_custom_target(junit
    COMMAND ${CMAKE_COMMAND}
        -DEXECUTABLE=$<TARGET_FILE:${PROJECT_NAME}Tests>
        -DDIR=${CMAKE_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_junit.cmake
    DEPENDS ${PROJECT_NAME}Tests
    COMMENT "Running tests with JUnit XML output"
)

# Each registered executable runs through run_junit.cmake as a POST_BUILD step
# on the `junit` custom target, so its cpputest_<group>.xml lands in the build
# dir alongside SolidSyslogTests' output. CI's junit-* upload-artifact then
# picks them all up for the Quality Summary panel. The `if(TARGET ...)` guard
# keeps the list authoritative without duplicating the FREERTOS_KERNEL_PATH /
# FREERTOS_PLUS_TCP_PATH env-var guards that already gate the test
# executables' definition in Tests/FreeRtos/CMakeLists.txt — a missing target
# is silently skipped, so jobs running outside a FreeRTOS-aware container
# just run SolidSyslogTests as before.
function(register_junit_test test_target)
    add_custom_command(TARGET junit POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DEXECUTABLE=$<TARGET_FILE:${test_target}>
            -DDIR=${CMAKE_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_junit.cmake
        COMMENT "Running ${test_target} with JUnit XML output"
        VERBATIM
    )
    add_dependencies(junit ${test_target})
endfunction()

foreach(freertos_test_target IN ITEMS
    SolidSyslogFreeRtosMutexTest
    SolidSyslogFreeRtosSysUpTimeTest
    CmsdkUartTest
    SolidSyslogFreeRtosDatagramTest
    SolidSyslogFreeRtosTcpStreamTest
    SolidSyslogFreeRtosStaticResolverTest
)
    if(TARGET ${freertos_test_target})
        register_junit_test(${freertos_test_target})
    endif()
endforeach()

# Code coverage report using lcov/genhtml.
# Requires the coverage preset: cmake --preset coverage && cmake --build --preset coverage --target coverage
# lcov and genhtml must be installed (available in both container images).
if(COVERAGE)
    find_program(LCOV_EXECUTABLE lcov)
    find_program(GENHTML_EXECUTABLE genhtml)

    if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
        add_custom_target(coverage
            COMMAND ${LCOV_EXECUTABLE} --zerocounters --directory ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:${PROJECT_NAME}Tests>
                -DDIR=${CMAKE_BINARY_DIR}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_junit.cmake
            COMMAND ${LCOV_EXECUTABLE} --capture --directory ${CMAKE_BINARY_DIR}
                --output-file ${CMAKE_BINARY_DIR}/coverage.info
                --rc branch_coverage=1
            COMMAND ${LCOV_EXECUTABLE} --extract ${CMAKE_BINARY_DIR}/coverage.info
                "${CMAKE_SOURCE_DIR}/Core/Source/*"
                "${CMAKE_SOURCE_DIR}/Platform/*/Source/*"
                --output-file ${CMAKE_BINARY_DIR}/coverage.filtered.info
                --rc branch_coverage=1
            COMMAND ${GENHTML_EXECUTABLE} ${CMAKE_BINARY_DIR}/coverage.filtered.info
                --output-directory ${CMAKE_BINARY_DIR}/coverage_report
                --branch-coverage
            DEPENDS ${PROJECT_NAME}Tests
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating coverage report in build/coverage/coverage_report/"
        )
    else()
        message(STATUS "lcov/genhtml not found — 'coverage' target unavailable (requires updated base image)")
    endif()
endif()
