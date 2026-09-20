# Tests/LittleFs - host-side unit tests for the LittleFS adapter under
# Platform/LittleFs/Source/. The executable recompiles the adapter source
# against the real lfs.h with the lfs_* functions faked, the same shape as
# Tests/FatFs/.
#
# Gated by $LITTLEFS_PATH at the top-level CMakeLists.

add_executable(SolidSyslogLittleFsFileTest
    SolidSyslogLittleFsFileTest.cpp
    SolidSyslogLittleFsFilePoolTest.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/Platform/LittleFs/Source/SolidSyslogLittleFsFile.c
    ${CMAKE_SOURCE_DIR}/Platform/LittleFs/Source/SolidSyslogLittleFsFileStatic.c
    ${CMAKE_SOURCE_DIR}/Tests/Support/LittleFsFakes/Source/LittleFsFake.c
)

target_link_libraries(SolidSyslogLittleFsFileTest PRIVATE
    ${PROJECT_NAME}
    ErrorHandlerFake
    ConfigLockFake
    CppUTest
    CppUTestExt
)

target_include_directories(SolidSyslogLittleFsFileTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/LittleFs/Interface
    ${CMAKE_SOURCE_DIR}/Core/Interface
    ${CMAKE_SOURCE_DIR}/Core/Source
    ${CMAKE_SOURCE_DIR}/Tests/Support
    ${CMAKE_SOURCE_DIR}/Tests/Support/LittleFsFakes/Interface
    ${CMAKE_SOURCE_DIR}/Tests/Support/LittleFsFakes/Source
)

# LittleFS's own lfs_util.h does not survive -Wsign-conversion, which our bar
# sets and which upstream does not build with. SYSTEM silences warnings from
# the tree without relaxing anything we compile ourselves. An integrator
# building the pack under the same bar needs the same treatment - said in
# docs/platforms/littlefs/setup.md.
target_include_directories(SolidSyslogLittleFsFileTest SYSTEM PRIVATE
    ${SOLIDSYSLOG_LITTLEFS_PATH}
)

add_test(NAME SolidSyslogLittleFsFileTest COMMAND SolidSyslogLittleFsFileTest)
