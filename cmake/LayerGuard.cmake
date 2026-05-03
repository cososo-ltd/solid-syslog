# Layer guard — fails configure if the source tree violates layering rules:
#
#   - Core/      must not include headers owned by Platform/* or Example/*.
#   - Platform/* must not include headers owned by Example/*.
#
# The check operates on header basenames: it enumerates every *.h owned by a
# disallowed layer, then scans local-form `#include "..."` directives in the
# consuming layer for any match. Core can freely include Core/Interface
# headers (e.g. SolidSyslogAddress.h) because those live in Core, not Platform.
#
# Tests/ and Bdd/ are out of scope (per CLAUDE.md tier table) and are not
# scanned — test code may include anything it needs.
#
# CI re-runs configure on every build, so PRs are always checked. Locally,
# violations are caught on the next reconfigure (typically when CMakeLists.txt
# changes, or via `cmake --preset <p>`).

function(_solidsyslog_collect_header_names OUT_VAR)
    set(_names "")
    foreach(_dir ${ARGN})
        file(GLOB_RECURSE _hdrs "${_dir}/*.h")
        foreach(_h ${_hdrs})
            get_filename_component(_n "${_h}" NAME)
            list(APPEND _names "${_n}")
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES _names)
    set(${OUT_VAR} "${_names}" PARENT_SCOPE)
endfunction()

function(_solidsyslog_scan_layer LAYER_LABEL FORBIDDEN_NAMES OUT_VIOLATIONS)
    set(_local "${${OUT_VIOLATIONS}}")
    foreach(_dir ${ARGN})
        file(GLOB_RECURSE _files "${_dir}/*.c" "${_dir}/*.h")
        foreach(_f ${_files})
            file(STRINGS "${_f}" _lines REGEX "^[ \t]*#[ \t]*include[ \t]+\"[^\"]+\"")
            foreach(_line ${_lines})
                string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" _path "${_line}")
                get_filename_component(_n "${_path}" NAME)
                if(_n IN_LIST FORBIDDEN_NAMES)
                    file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}" "${_f}")
                    list(APPEND _local "  [${LAYER_LABEL}] ${_rel}: #include \"${_path}\"")
                endif()
            endforeach()
        endforeach()
    endforeach()
    set(${OUT_VIOLATIONS} "${_local}" PARENT_SCOPE)
endfunction()

function(solidsyslog_enforce_layering)
    _solidsyslog_collect_header_names(_platform_names
        "${CMAKE_SOURCE_DIR}/Platform"
    )
    _solidsyslog_collect_header_names(_example_names
        "${CMAKE_SOURCE_DIR}/Example"
    )

    set(_forbidden_in_core "${_platform_names}")
    list(APPEND _forbidden_in_core ${_example_names})
    list(REMOVE_DUPLICATES _forbidden_in_core)

    set(_violations "")

    _solidsyslog_scan_layer("Core" "${_forbidden_in_core}" _violations
        "${CMAKE_SOURCE_DIR}/Core"
    )
    _solidsyslog_scan_layer("Platform" "${_example_names}" _violations
        "${CMAKE_SOURCE_DIR}/Platform"
    )

    if(_violations)
        list(JOIN _violations "\n" _msg)
        message(FATAL_ERROR
            "Layering rule violation(s):\n${_msg}\n"
            "Core must not include from Platform/ or Example/. "
            "Platform must not include from Example/.")
    endif()
endfunction()
