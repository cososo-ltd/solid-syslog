# FreeRtosFakes — host-side fake implementations of the FreeRTOS API used by
# Tests/FreeRtos/*Test executables. Real FreeRTOS-Kernel headers come from
# $FREERTOS_KERNEL_PATH/include; the test FreeRTOSConfig.h lives in this
# directory's Interface/. Drift between fakes and the real FreeRTOS API is
# caught at compile time.
#
# Empty at S08.01 — the directory structure is in place so later stories
# (S08.04 mutex, S08.05 file, S08.07 TLS) just add fake source files. CMake
# allows an empty subdirectory and only requires this file to exist.
