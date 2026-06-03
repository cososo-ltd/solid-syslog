# Tests/PlusFat — host-side unit tests for FreeRTOS-Plus-FAT adapters under
# Platform/PlusFat/Source/. Each test executable recompiles the adapter source
# under test with the PlusFatFakes config and links against the fakes — same
# shape as Tests/FatFs/ and Tests/FreeRtos/, see
# project_header_configured_platforms memory.
#
# Plus-FAT is FreeRTOS-coupled: ff_stdio.h pulls FreeRTOS.h / task.h / semphr.h,
# so the include path carries FreeRtosFakes/Interface (FreeRTOSConfig.h,
# portmacro.h) and $FREERTOS_KERNEL_PATH/include alongside the test-local
# FreeRTOSFATConfig.h in PlusFatFakes/Interface. We surface that coupling
# rather than hide it.
#
# Gated by $FREERTOS_PLUS_FAT_PATH at the top-level CMakeLists (this CMakeLists
# is only entered when FREERTOS_PLUS_FAT_PATH is set), and additionally needs
# FREERTOS_KERNEL_PATH for the kernel headers Plus-FAT depends on.

if("$ENV{FREERTOS_KERNEL_PATH}" STREQUAL "")
    message(STATUS "Skipping SolidSyslogPlusFatFileTest: FREERTOS_KERNEL_PATH not set.")
else()
    add_executable(SolidSyslogPlusFatFileTest
        SolidSyslogPlusFatFileTest.cpp
        SolidSyslogPlusFatFilePoolTest.cpp
        main.cpp
        ${CMAKE_SOURCE_DIR}/Platform/PlusFat/Source/SolidSyslogPlusFatFile.c
        ${CMAKE_SOURCE_DIR}/Platform/PlusFat/Source/SolidSyslogPlusFatFileStatic.c
        ${CMAKE_SOURCE_DIR}/Tests/Support/PlusFatFakes/Source/PlusFatFake.c
    )

    target_link_libraries(SolidSyslogPlusFatFileTest PRIVATE
        ${PROJECT_NAME}
        ErrorHandlerFake
        ConfigLockFake
        CppUTest
        CppUTestExt
    )

    target_include_directories(SolidSyslogPlusFatFileTest PRIVATE
        ${CMAKE_SOURCE_DIR}/Platform/PlusFat/Interface
        ${CMAKE_SOURCE_DIR}/Core/Interface
        ${CMAKE_SOURCE_DIR}/Core/Source
        ${CMAKE_SOURCE_DIR}/Tests/Support
        ${CMAKE_SOURCE_DIR}/Tests/Support/PlusFatFakes/Interface
        ${CMAKE_SOURCE_DIR}/Tests/Support/PlusFatFakes/Source
        ${CMAKE_SOURCE_DIR}/Tests/Support/FreeRtosFakes/Interface
        $ENV{FREERTOS_KERNEL_PATH}/include
        $ENV{FREERTOS_PLUS_FAT_PATH}/include
    )

    add_test(NAME SolidSyslogPlusFatFileTest COMMAND SolidSyslogPlusFatFileTest)
endif()
