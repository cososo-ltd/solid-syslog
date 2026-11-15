# The last step of the `junit` target: fails it if any executable's run left a
# marker in FAILURES_DIR, naming each, so one failure never hides another.
file(GLOB failed RELATIVE ${FAILURES_DIR} ${FAILURES_DIR}/*)
if(failed)
    list(JOIN failed ", " failed)
    message(FATAL_ERROR "Tests failed in: ${failed}")
endif()
