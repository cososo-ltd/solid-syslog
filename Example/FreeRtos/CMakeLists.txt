# Top-level descender for the FreeRTOS example tree. Only entered by the
# top-level CMakeLists.txt when CMAKE_CROSSCOMPILING is true and the system
# processor is arm — i.e. via the `freertos-cross` preset.

add_subdirectory(HelloWorld)
