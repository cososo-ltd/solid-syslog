# Tests/FreeRtos — host-side unit tests for FreeRTOS adapters under
# Platform/FreeRtos/Source/. Each test executable recompiles the adapter
# source under test with the FreeRtosFakes config and links against the
# fakes — see project_header_configured_platforms memory.

# Top-level CMakeLists.txt only checks FREERTOS_KERNEL_PATH before adding this
# subdirectory. Adapter tests transitively include FreeRTOS-Plus-TCP headers,
# so guard FREERTOS_PLUS_TCP_PATH here too — fail loud rather than expand to
# a stray "/source/include" suffix on an empty path.
if("$ENV{FREERTOS_PLUS_TCP_PATH}" STREQUAL "")
    message(FATAL_ERROR "FREERTOS_PLUS_TCP_PATH must be set to build Tests/FreeRtos.")
endif()

add_executable(SolidSyslogFreeRtosDatagramTest
    SolidSyslogFreeRtosDatagramTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/FreeRtos/Source/SolidSyslogFreeRtosDatagram.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/FreeRtosFakes/Source/FreeRtosSocketsFake.c
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
    ${CMAKE_SOURCE_DIR}/Tests/Support/FreeRtosFakes/Interface
    $ENV{FREERTOS_KERNEL_PATH}/include
    $ENV{FREERTOS_PLUS_TCP_PATH}/source/include
)

add_test(NAME SolidSyslogFreeRtosDatagramTest COMMAND SolidSyslogFreeRtosDatagramTest)

# CMSDK UART driver host tests — recompiles the driver source from the
# Example/FreeRtos/HelloWorld board glue against an in-memory fake register
# block. No FreeRTOS-Plus-TCP involvement, so the include path is intentionally
# narrower than the datagram test.
add_executable(CmsdkUartTest
    CmsdkUartTest.cpp
    CmsdkUartFake.c
    main.cpp
    ${CMAKE_SOURCE_DIR}/Example/FreeRtos/HelloWorld/CmsdkUart.c
)

target_link_libraries(CmsdkUartTest PRIVATE
    CppUTest
    CppUTestExt
)

target_include_directories(CmsdkUartTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Example/FreeRtos/HelloWorld
)

add_test(NAME CmsdkUartTest COMMAND CmsdkUartTest)
