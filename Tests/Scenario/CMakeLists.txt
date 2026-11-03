# The scenario suite: the real Core over the fakes, driven by scripted fault
# shapes and seeded random walks, with the invariants checked after every
# pass. A separate executable so the per-commit `junit` run stays fast: it
# takes its walk count from SOLIDSYSLOG_SCENARIO_RUNS (default a smoke
# count), and SOLIDSYSLOG_SCENARIO_SEED re-runs one walk by the seed a
# failure printed. CI exports the full count in the lanes that carry it.
add_executable(SolidSyslogScenarioSuite
    main.cpp
    ScenarioInvariants.cpp
    ScenarioSuite.cpp
    ScenarioSuiteTest.cpp
)

# The count and seed come from the environment, which MSVC reads only
# through getenv_s. CMake picks the implementation as it does SafeString's.
if(MSVC)
    target_sources(SolidSyslogScenarioSuite PRIVATE ${CMAKE_SOURCE_DIR}/Tests/Support/EnvironmentWindows.c)
else()
    target_sources(SolidSyslogScenarioSuite PRIVATE ${CMAKE_SOURCE_DIR}/Tests/Support/EnvironmentStandard.c)
endif()

target_link_libraries(SolidSyslogScenarioSuite PRIVATE
    ${PROJECT_NAME}
    ScenarioBench
    CppUTest
    CppUTestExt
    Threads::Threads
)

solidsyslog_add_test(SolidSyslogScenarioSuite)
