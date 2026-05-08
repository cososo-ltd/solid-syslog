# FreeRTOS hello-world ELF for QEMU mps2-an385 (Cortex-M3).
#
# Compiled cross-only — gated by the top-level CMakeLists.txt on
# CMAKE_CROSSCOMPILING + CMAKE_SYSTEM_PROCESSOR == "arm".

set(FREERTOS_KERNEL_PATH "$ENV{FREERTOS_KERNEL_PATH}")
if(NOT FREERTOS_KERNEL_PATH)
    message(FATAL_ERROR
        "FREERTOS_KERNEL_PATH not set. Use the cpputest-freertos-cross "
        "container, which sets this to /opt/freertos/kernel.")
endif()

set(FREERTOS_PORT_DIR "${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM3")

add_executable(SolidSyslogFreeRtosHelloWorld
    main.c
    startup.c
    CmsdkUart.c
    Syscalls.c
    ${FREERTOS_KERNEL_PATH}/tasks.c
    ${FREERTOS_KERNEL_PATH}/queue.c
    ${FREERTOS_KERNEL_PATH}/list.c
    ${FREERTOS_PORT_DIR}/port.c
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
)

set_target_properties(SolidSyslogFreeRtosHelloWorld PROPERTIES SUFFIX ".elf")

target_compile_options(SolidSyslogFreeRtosHelloWorld PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -fno-common
    # FreeRTOS-Kernel V11.1.0 uses non-prototype function declarations and
    # type-narrowing constructs that trip our strict host warnings; relax
    # for the kernel-included sources without lowering the bar everywhere.
    -Wno-unused-parameter
)

target_include_directories(SolidSyslogFreeRtosHelloWorld PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}            # FreeRTOSConfig.h
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_PORT_DIR}                   # portmacro.h
)

target_link_options(SolidSyslogFreeRtosHelloWorld PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -T${CMAKE_CURRENT_SOURCE_DIR}/mps2-an385.ld
    # nano: size-optimised newlib variant.
    # nosys: empty syscall stubs for symbols Syscalls.c does not override
    #        (open, unlink, etc.); our _write/_read/_sbrk wins as a strong
    #        symbol, so printf routes to CmsdkUart over QEMU `-serial stdio`.
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE:SolidSyslogFreeRtosHelloWorld>.map
)

# Ensure CMake re-links when the linker script changes.
set_target_properties(SolidSyslogFreeRtosHelloWorld PROPERTIES
    LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mps2-an385.ld)
