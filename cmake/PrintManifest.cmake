# Helper for the `manifest` custom target: print the generated manifest to
# stdout. Run in script mode (cmake -P) so it works on the project's CMake 3.16
# floor (cmake -E cat is 3.18+).
if(NOT DEFINED SOLIDSYSLOG_MANIFEST_FILE OR NOT EXISTS "${SOLIDSYSLOG_MANIFEST_FILE}")
    message(FATAL_ERROR "Manifest file not found: ${SOLIDSYSLOG_MANIFEST_FILE}")
endif()
file(READ "${SOLIDSYSLOG_MANIFEST_FILE}" _contents)
message("${_contents}")
