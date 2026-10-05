# Runs the test executable with verbose JUnit XML output.
# The XML is written regardless of pass/fail, then the exit code is propagated
# so cmake --build --target junit fails when tests fail.
file(MAKE_DIRECTORY ${DIR})
execute_process(
    COMMAND ${EXECUTABLE} -v -ojunit
    WORKING_DIRECTORY ${DIR}
    RESULT_VARIABLE TEST_RESULT
)
if(NOT TEST_RESULT EQUAL 0)
    message(FATAL_ERROR "Tests failed")
endif()
