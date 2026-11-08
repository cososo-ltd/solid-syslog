# Runs the test executable with verbose JUnit XML output.
# The XML is written regardless of pass/fail, then the exit code is propagated
# so cmake --build --target junit fails when tests fail.
#
# PACKAGE is the executable's target name. CppUTest puts it in front of every
# group's file name (cpputest_<Package>_<Group>.xml) and classname
# (<Package>.<Group>), which is how the test report the documentation publishes
# tells one executable's suites from another's.
file(MAKE_DIRECTORY ${DIR})
set(package_args)
if(PACKAGE)
    set(package_args -k ${PACKAGE})
endif()
execute_process(
    COMMAND ${EXECUTABLE} -v -ojunit ${package_args}
    WORKING_DIRECTORY ${DIR}
    RESULT_VARIABLE TEST_RESULT
)
if(NOT TEST_RESULT EQUAL 0)
    message(FATAL_ERROR "Tests failed")
endif()
