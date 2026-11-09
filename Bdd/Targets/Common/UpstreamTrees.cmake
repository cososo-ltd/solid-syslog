# The upstream trees a BDD target wires, resolved and recorded in one place.
#
# Every cross target takes its upstream sources from paths the cross image
# pins: SOLIDSYSLOG_FREERTOS_KERNEL_PATH, SOLIDSYSLOG_LWIP_PATH and the rest,
# each with an environment fallback of the same name less the prefix. The
# image pins every tree for every build, so the configure alone cannot say
# which of them a given target reads. This function can: each target resolves
# every tree it wires through it, and the configure leaves a list of them at
# ${CMAKE_BINARY_DIR}/SolidSyslogUpstreamTrees.txt, one variable name per
# line. scripts/check_build_provenance.py reads that list after the build and
# fails if a file came from any other tree, or if a listed tree went unread.
#
#   solidsyslog_bdd_upstream(<local> <variable> <pinned path> [PROBE <file>])
#
# Sets <local> to the value of <variable>, failing the configure with the
# message every target used to carry if it is empty, or if PROBE names a file
# relative to it that does not exist. <pinned path> is where the image sets
# it, for that message.

file(WRITE "${CMAKE_BINARY_DIR}/SolidSyslogUpstreamTrees.txt" "")

function(solidsyslog_bdd_upstream local variable pinned_path)
    cmake_parse_arguments(ARG "" "PROBE" "" ${ARGN})
    string(REPLACE "SOLIDSYSLOG_" "" environment_fallback "${variable}")
    set(value "${${variable}}")
    if(NOT value)
        message(FATAL_ERROR
            "${variable} is empty (falls back to $${environment_fallback}). Use the "
            "cpputest-freertos-cross container, which sets this to ${pinned_path}.")
    endif()
    if(ARG_PROBE AND NOT EXISTS "${value}/${ARG_PROBE}")
        message(FATAL_ERROR
            "${variable} is invalid: ${value}/${ARG_PROBE} does not exist (falls back to "
            "$${environment_fallback}). Use the cpputest-freertos-cross container, which sets this to "
            "${pinned_path}.")
    endif()
    file(APPEND "${CMAKE_BINARY_DIR}/SolidSyslogUpstreamTrees.txt" "${variable}\n")
    set(${local} "${value}" PARENT_SCOPE)
endfunction()
