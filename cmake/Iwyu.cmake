# Include-what-you-use (IWYU) opt-in.
#
# When ENABLE_IWYU is ON, two custom targets are defined:
#
#   iwyu       — runs IWYU over Core/, Platform/, Example/. Pipes IWYU's
#                output through scripts/iwyu_filter.py, which drops
#                redundant-forward-declaration findings (see filter docstring
#                for the MISRA rationale). Exits non-zero if any actionable
#                finding remains.
#   iwyu-apply — applies the surviving suggestions via fix_includes.py.
#                Used during cleanup, never invoked from CI.
#
# Bdd/ is out of scope per CLAUDE.md tier table. Tests/ is included even
# though it's Tier 4 ("out of scope for support") because narrowing the
# public Core headers exposes test code that was relying on transitive
# includes; the same hygiene rule needs to hold there or the gate keeps
# tripping consumers.
#
# We don't use CMake's CMAKE_<LANG>_INCLUDE_WHAT_YOU_USE launcher mechanism
# because CMake (since 3.11) intentionally ignores the launcher's exit code,
# so `--error` would not fail the build.

option(ENABLE_IWYU "Define iwyu/iwyu-apply targets that run include-what-you-use over Core/Platform/Example" OFF)

if(ENABLE_IWYU)
    find_program(IWYU_EXECUTABLE NAMES include-what-you-use REQUIRED)
    find_program(IWYU_TOOL       NAMES iwyu_tool.py iwyu_tool REQUIRED)
    find_program(IWYU_FIX        NAMES fix_includes.py fix_includes REQUIRED)
    find_program(BASH_EXECUTABLE NAMES bash REQUIRED)

    set(_iwyu_mapping_dir "/usr/local/share/include-what-you-use")
    set(_iwyu_filter "${CMAKE_SOURCE_DIR}/scripts/iwyu_filter.py")

    # No explicit source-file list — iwyu_tool.py iterates every entry in
    # compile_commands.json, so the active configuration's sources are exactly
    # what gets analysed. (Listing files explicitly produced spurious "not found
    # in compilation database" warnings for Windows-only sources on Linux.)
    # IWYU's own --error flag is intentionally NOT set: with --error, IWYU
    # exits non-zero on any finding, including the filtered-out classes
    # (forward decls, dual-language headers, CppUTest macros). That non-zero
    # exit propagates through iwyu_tool.py and pipefail before our filter
    # gets a chance to suppress benign findings. Instead, IWYU always exits 0
    # and scripts/iwyu_filter.py is the authoritative gate.
    set(_iwyu_invocation
        "${IWYU_TOOL} -p ${CMAKE_BINARY_DIR} -- \
-Xiwyu --check_also=*Interface/*.h \
-Xiwyu --check_also=*Example/*/*.h \
-Xiwyu --check_also=*Tests/*.h \
-Xiwyu --mapping_file=${_iwyu_mapping_dir}/gcc.libc.imp \
-Xiwyu --mapping_file=${_iwyu_mapping_dir}/stl.c.headers.imp \
-Xiwyu --mapping_file=${_iwyu_mapping_dir}/stl.public.imp \
-Xiwyu --mapping_file=${CMAKE_SOURCE_DIR}/cmake/cpputest.imp"
    )

    add_custom_target(iwyu
        COMMAND "${BASH_EXECUTABLE}" -c
            "set -o pipefail; ${_iwyu_invocation} | python3 ${_iwyu_filter}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running include-what-you-use (filtered) over Core/, Platform/, Example/"
        VERBATIM
        USES_TERMINAL
    )

    add_custom_target(iwyu-apply
        COMMAND "${BASH_EXECUTABLE}" -c
            "${_iwyu_invocation} | python3 ${_iwyu_filter} --filter-only | ${IWYU_FIX} --nosafe_headers"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Applying include-what-you-use suggestions (filtered)"
        VERBATIM
        USES_TERMINAL
    )
endif()
