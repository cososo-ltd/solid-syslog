# The kernel half of the CmsisLwip target: FreeRTOS, plus the CMSIS-RTOS2
# wrapper that presents it through the API the target's own code speaks.
#
# Everything here is what a kernel swap would replace, and nothing outside this
# directory names FreeRTOS except the lwIP port layer and the Ethernet driver,
# which sit beneath the wrapper. A second kernel is a sibling directory
# supplying the same three names below, and a one-line change to which
# Kernel.cmake the target includes.
#
# Sets, for the target to consume:
#   BDD_KERNEL_UPSTREAM_SOURCES  - third-party sources, compiled under relaxed
#                                  warnings in the target's OBJECT library
#   BDD_KERNEL_UPSTREAM_INCLUDES - include dirs those sources need
#   BDD_KERNEL_PROJECT_SOURCES   - our own sources, compiled under the strict bar

set(BDD_KERNEL_DIR ${CMAKE_CURRENT_LIST_DIR})

# The upstream trees this kernel wires - see Bdd/Targets/Common/UpstreamTrees.cmake.
# CMSIS-RTOS2 needs both CMSIS repositories and says so only when the build
# fails: CMSIS-FreeRTOS ships cmsis_os2.c but no cmsis_os2.h, and the header
# plus os_tick.h and cmsis_compiler.h come from CMSIS_6. The two are
# version-paired with the kernel - bump all three together or none.
solidsyslog_bdd_upstream(FREERTOS_KERNEL_PATH SOLIDSYSLOG_FREERTOS_KERNEL_PATH /opt/freertos/kernel)
solidsyslog_bdd_upstream(CMSIS_PATH SOLIDSYSLOG_CMSIS_PATH /opt/cmsis)
solidsyslog_bdd_upstream(CMSIS_FREERTOS_PATH SOLIDSYSLOG_CMSIS_FREERTOS_PATH /opt/cmsis-freertos)

set(FREERTOS_PORT_DIR "${FREERTOS_KERNEL_PATH}/portable/${BDD_BOARD_FREERTOS_PORT}")

set(BDD_KERNEL_UPSTREAM_SOURCES
    ${FREERTOS_KERNEL_PATH}/tasks.c
    ${FREERTOS_KERNEL_PATH}/queue.c
    ${FREERTOS_KERNEL_PATH}/list.c
    ${FREERTOS_KERNEL_PATH}/timers.c
    ${FREERTOS_KERNEL_PATH}/event_groups.c
    ${FREERTOS_PORT_DIR}/port.c
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
    # The CMSIS-RTOS2 wrapper over the kernel above. Third-party, so it sits
    # under the same relaxations rather than our strict warning bar.
    ${CMSIS_FREERTOS_PATH}/CMSIS/RTOS2/FreeRTOS/Source/cmsis_os2.c
)

# Outside a CMSIS-Pack RTE build the wrapper will not compile without a device
# header named by CMSIS_device_header. Naming one also defines SysTick, so the
# wrapper supplies SysTick_Handler itself and calls the port's handler from it.
set_property(
    SOURCE ${CMSIS_FREERTOS_PATH}/CMSIS/RTOS2/FreeRTOS/Source/cmsis_os2.c
    APPEND PROPERTY COMPILE_DEFINITIONS "CMSIS_device_header=\"BddTargetCmsisDevice.h\""
)

set(BDD_KERNEL_UPSTREAM_INCLUDES
    ${BDD_KERNEL_DIR}                                      # FreeRTOSConfig.h
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_PORT_DIR}                                   # portmacro.h
    ${CMSIS_PATH}/CMSIS/RTOS2/Include                      # cmsis_os2.h, os_tick.h
    ${CMSIS_PATH}/CMSIS/Core/Include                       # cmsis_compiler.h
    ${CMSIS_FREERTOS_PATH}/CMSIS/RTOS2/FreeRTOS/Include    # freertos_os2.h, freertos_mpool.h
)

set(BDD_KERNEL_PROJECT_SOURCES
    ${BDD_KERNEL_DIR}/KernelHooks.c
)
