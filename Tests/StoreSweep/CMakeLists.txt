# The store sweep: every single fault at every position on the media, judged
# by the store bench's verdicts. A separate executable so the per-commit
# `junit` run stays fast: it takes its case budget from
# SOLIDSYSLOG_STORE_SWEEP_BUDGET (default a smoke budget of a few hundred
# cases, 0 for every case), and SOLIDSYSLOG_STORE_SWEEP_CASE re-runs one case
# by its tuple. CI exports the full budget in the lanes that carry it.
add_executable(SolidSyslogStoreSweep main.cpp StoreSweepTest.cpp)

target_link_libraries(SolidSyslogStoreSweep PRIVATE
    ${PROJECT_NAME}
    StoreBench
    BlockDeviceFake
    CppUTest
    CppUTestExt
    Threads::Threads
)

solidsyslog_add_test(SolidSyslogStoreSweep)
