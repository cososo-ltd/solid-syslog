# Turns the raw profiles a coverage run leaves into the files the gate, the
# documentation site and a reader each want. Run by the `coverage` target:
#
#   cmake -DLLVM_PROFDATA=... -DLLVM_COV=... -DPROFRAW_DIR=... -DSOURCE_DIR=...
#         -DOUTPUT_DIR=... -P llvm_cov.cmake <test executable>...
#
# Leaves in OUTPUT_DIR:
#   coverage.profdata        the merged profile
#   coverage.filtered.info   lcov tracefile, lines and branches, for genhtml and
#                            scripts/coverage_summary.py
#   coverage.llvm.json       llvm-cov's per-file summary, which carries the
#                            MC/DC conditions the tracefile cannot
#   report/mcdc-report/      llvm-cov's HTML, each decision with its test vectors
#
# Only the production sources are reported: Core/Source and every pack's Source.
# The executables are every one `junit` ran, so each function is read from the
# binary that holds it.

set(objects)
set(after_script FALSE)
foreach(index RANGE 1 ${CMAKE_ARGC})
    if(after_script)
        list(APPEND objects ${CMAKE_ARGV${index}})
    elseif("${CMAKE_ARGV${index}}" STREQUAL "${CMAKE_SCRIPT_MODE_FILE}")
        set(after_script TRUE)
    endif()
endforeach()
if(NOT objects)
    message(FATAL_ERROR "No test executables were named after the script")
endif()

file(GLOB profiles "${PROFRAW_DIR}/*.profraw")
if(NOT profiles)
    message(FATAL_ERROR "No raw profiles in ${PROFRAW_DIR}: did the tests run under the coverage preset?")
endif()

set(sources ${SOURCE_DIR}/Core/Source)
file(GLOB pack_sources LIST_DIRECTORIES true ${SOURCE_DIR}/Platform/*/Source)
list(APPEND sources ${pack_sources})

# The first executable is llvm-cov's positional binary; the rest are -object.
list(POP_FRONT objects first_object)
set(object_args ${first_object})
foreach(object IN LISTS objects)
    list(APPEND object_args -object ${object})
endforeach()

set(profdata ${OUTPUT_DIR}/coverage.profdata)
execute_process(
    COMMAND ${LLVM_PROFDATA} merge -sparse -o ${profdata} ${profiles}
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND ${LLVM_COV} export ${object_args} -instr-profile=${profdata} -format=lcov ${sources}
    OUTPUT_FILE ${OUTPUT_DIR}/coverage.filtered.info
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND ${LLVM_COV} export ${object_args} -instr-profile=${profdata} -format=text -summary-only ${sources}
    OUTPUT_FILE ${OUTPUT_DIR}/coverage.llvm.json
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND ${LLVM_COV} show ${object_args} -instr-profile=${profdata}
        -format=html -output-dir=${OUTPUT_DIR}/report/mcdc-report
        -show-mcdc -show-branches=count -show-line-counts-or-regions
        ${sources}
    COMMAND_ERROR_IS_FATAL ANY
)
