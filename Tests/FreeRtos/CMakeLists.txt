# Tests/FreeRtos — host-side unit tests for FreeRTOS adapters under
# Platform/FreeRtos/Source/. Each test executable recompiles the adapter
# source under test with the FreeRtosFakes config and links against the
# fakes — see project_header_configured_platforms memory.

# SolidSyslogFreeRtosDatagramTest transitively includes FreeRTOS-Plus-TCP
# headers, so it requires FREERTOS_PLUS_TCP_PATH. CmsdkUartTest does not —
# it tests UART board glue and never touches the IP stack. Scope the guard
# to the datagram test so a minimal FREERTOS_KERNEL_PATH-only environment
# can still build the UART test.
if("$ENV{FREERTOS_PLUS_TCP_PATH}" STREQUAL "")
    message(STATUS "Skipping SolidSyslogFreeRtosDatagramTest: FREERTOS_PLUS_TCP_PATH not set.")
else()
    add_executable(SolidSyslogFreeRtosDatagramTest
        SolidSyslogFreeRtosDatagramTest.cpp
        main.cpp
        ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosDatagram.c
        ${CMAKE_SOURCE_DIR}/Tests/Support/FreeRtosFakes/Source/FreeRtosSocketsFake.c
        ${CMAKE_SOURCE_DIR}/Tests/Support/FreeRtosFakes/Source/FreeRtosArpFake.c
        ${CMAKE_SOURCE_DIR}/Tests/Support/FreeRtosFakes/Source/FreeRtosTaskFake.c
    )

    target_link_libraries(SolidSyslogFreeRtosDatagramTest PRIVATE
        ${PROJECT_NAME}
        CppUTest
        CppUTestExt
    )

    target_include_directories(SolidSyslogFreeRtosDatagramTest PRIVATE
        ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Interface
        ${CMAKE_SOURCE_DIR}/Core/Interface
        ${CMAKE_SOURCE_DIR}/Core/Source
        ${CMAKE_SOURCE_DIR}/Tests/Support
        ${CMAKE_SOURCE_DIR}/Tests/Support/FreeRtosFakes/Interface
        $ENV{FREERTOS_KERNEL_PATH}/include
        $ENV{FREERTOS_PLUS_TCP_PATH}/source/include
    )

    add_test(NAME SolidSyslogFreeRtosDatagramTest COMMAND SolidSyslogFreeRtosDatagramTest)

    add_executable(SolidSyslogFreeRtosStaticResolverTest
        SolidSyslogFreeRtosStaticResolverTest.cpp
        main.cpp
        ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosStaticResolver.c
    )

    target_link_libraries(SolidSyslogFreeRtosStaticResolverTest PRIVATE
        ${PROJECT_NAME}
        CppUTest
        CppUTestExt
    )

    target_include_directories(SolidSyslogFreeRtosStaticResolverTest PRIVATE
        ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Interface
        ${CMAKE_SOURCE_DIR}/Core/Interface
        ${CMAKE_SOURCE_DIR}/Core/Source
        ${CMAKE_SOURCE_DIR}/Tests/Support
        ${CMAKE_SOURCE_DIR}/Tests/Support/FreeRtosFakes/Interface
        $ENV{FREERTOS_KERNEL_PATH}/include
        $ENV{FREERTOS_PLUS_TCP_PATH}/source/include
    )

    add_test(NAME SolidSyslogFreeRtosStaticResolverTest COMMAND SolidSyslogFreeRtosStaticResolverTest)
endif()

# SolidSyslogFreeRtosMutex — kernel-only adapter wrapping a FreeRTOS static
# mutex semaphore; needs FREERTOS_KERNEL_PATH but not Plus-TCP. Sits outside
# the Plus-TCP guard for the same reason the SysUpTime / UART tests do.
add_executable(SolidSyslogFreeRtosMutexTest
    SolidSyslogFreeRtosMutexTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosMutex.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/FreeRtosFakes/Source/FreeRtosSemaphoreFake.c
)

target_link_libraries(SolidSyslogFreeRtosMutexTest PRIVATE
    ${PROJECT_NAME}
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogFreeRtosMutexTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests/Support
    ${CMAKE_SOURCE_DIR}/Tests/Support/FreeRtosFakes/Interface
    $ENV{FREERTOS_KERNEL_PATH}/include
)

add_test(NAME SolidSyslogFreeRtosMutexTest COMMAND SolidSyslogFreeRtosMutexTest)

# SolidSyslogFreeRtosSysUpTime — kernel-only adapter; needs FREERTOS_KERNEL_PATH
# but not Plus-TCP. Sits outside the Plus-TCP guard for the same reason the
# UART test does: a minimal kernel-only environment must still build it.
add_executable(SolidSyslogFreeRtosSysUpTimeTest
    SolidSyslogFreeRtosSysUpTimeTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosSysUpTime.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/FreeRtosFakes/Source/FreeRtosTaskFake.c
)

target_link_libraries(SolidSyslogFreeRtosSysUpTimeTest PRIVATE
    ${PROJECT_NAME}
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogFreeRtosSysUpTimeTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Tests/Support/FreeRtosFakes/Interface
    $ENV{FREERTOS_KERNEL_PATH}/include
)

add_test(NAME SolidSyslogFreeRtosSysUpTimeTest COMMAND SolidSyslogFreeRtosSysUpTimeTest)

# CMSDK UART driver host tests — recompiles the driver source from the
# Bdd/Targets/FreeRtos/Common board glue against an in-memory fake register
# block. No FreeRTOS-Plus-TCP involvement, so the include path is intentionally
# narrower than the datagram test.
add_executable(CmsdkUartTest
    CmsdkUartTest.cpp
    CmsdkUartFake.c
    main.cpp
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/FreeRtos/Common/CmsdkUart.c
)

target_link_libraries(CmsdkUartTest PRIVATE
    CppUTest
    CppUTestExt
)

target_include_directories(CmsdkUartTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Bdd/Targets/FreeRtos/Common
    ${CMAKE_SOURCE_DIR}/Core/Interface                # ExternC.h (via CmsdkUart.h)
    ${CMAKE_SOURCE_DIR}/Tests/Support
)

add_test(NAME CmsdkUartTest COMMAND CmsdkUartTest)
