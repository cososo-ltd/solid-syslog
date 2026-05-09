# Top-level descender for the FreeRTOS example tree. Only entered by the
# top-level CMakeLists.txt when CMAKE_CROSSCOMPILING is true and the system
# processor is arm — i.e. via the `freertos-cross` preset.

# Shared infrastructure consumed by every example target in this tree
# (CMSDK UART driver, newlib retargeting, Cortex-M3 startup, mps2-an385
# linker script). Per-application CMakeLists.txt files reference these
# files via this variable; nothing in Common/ is its own build target.
set(SOLID_SYSLOG_FREERTOS_EXAMPLE_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Common)

add_subdirectory(HelloWorld)
add_subdirectory(SingleTask)
