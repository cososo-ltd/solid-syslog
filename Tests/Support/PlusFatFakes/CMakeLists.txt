# PlusFatFakes — host-side fake implementations of the FreeRTOS-Plus-FAT
# ff_stdio API used by Tests/PlusFat/*Test executables. The real Plus-FAT
# headers come from $FREERTOS_PLUS_FAT_PATH/include/; the test
# FreeRTOSFATConfig.h lives in this directory's Interface/. Drift between the
# fakes and the real Plus-FAT API is caught at compile time when the fake
# source includes ff_stdio.h.
#
# Plus-FAT is FreeRTOS-coupled (ff_headers.h pulls FreeRTOS.h / task.h /
# semphr.h), so each consuming test exe also puts FreeRtosFakes/Interface and
# $FREERTOS_KERNEL_PATH/include on its include path. We do not hide that
# coupling — it is intrinsic to Plus-FAT.
#
# The PlusFatFake.h control surface and PlusFatFake.c bodies grow per-slice as
# new ff_* calls enter the adapter. Each test exe pulls the fake source
# directly via add_executable — see Tests/FatFs/ and Tests/FreeRtos/ pattern.
