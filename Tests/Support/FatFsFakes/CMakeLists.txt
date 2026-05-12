# FatFsFakes — host-side fake implementations of the FatFs API used by
# Tests/FatFs/*Test executables. The real FatFs headers come from
# $FATFS_PATH/source/; the test ffconf.h lives in this directory's
# Interface/. Drift between fakes and the real FatFs API is caught at
# compile time when the fake source file includes ff.h.
#
# Empty at S08.05 slice 1 — only ffconf.h is in place so the placeholder
# Tests/FatFs/SolidSyslogFatFsFileTest exe compiles. The FatFsFake.h
# control surface and FatFsFake.c bodies land per-slice as new f_* calls
# enter the adapter (slice 2 onwards). Each test exe pulls fake sources
# directly via add_executable — see Tests/FreeRtos/ pattern.
