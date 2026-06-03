# Tests/FatFs — host-side unit tests for FatFs adapters under
# Platform/FatFs/Source/. Each test executable recompiles the adapter
# source under test with the FatFsFakes config and links against the
# fakes — same shape as Tests/FreeRtos/, see
# project_header_configured_platforms memory.
#
# Gated by $FATFS_PATH at the top-level CMakeLists (this CMakeLists is
# only entered when FATFS_PATH is set), so the placeholder below is safe
# to define unconditionally.

add_executable(SolidSyslogFatFsFileTest
    SolidSyslogFatFsFileTest.cpp
    SolidSyslogFatFsFilePoolTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/FatFs/Source/SolidSyslogFatFsFile.c
    ${CMAKE_SOURCE_DIR}/Platform/FatFs/Source/SolidSyslogFatFsFileStatic.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/FatFsFakes/Source/FatFsFake.c
)

target_link_libraries(SolidSyslogFatFsFileTest PRIVATE
    ${PROJECT_NAME}
    ErrorHandlerFake
    ConfigLockFake
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogFatFsFileTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/FatFs/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests/Support
    ${CMAKE_SOURCE_DIR}/Tests/Support/FatFsFakes/Interface
    ${CMAKE_SOURCE_DIR}/Tests/Support/FatFsFakes/Source
    $ENV{FATFS_PATH}/source
)

add_test(NAME SolidSyslogFatFsFileTest COMMAND SolidSyslogFatFsFileTest)
