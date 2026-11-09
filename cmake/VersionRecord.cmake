# What this configure compiled against, recorded for anyone reading a CI run.
#
# SUPPORT.md says a pack is supported when it is built and tested in CI against
# the upstream version its page names. This writes the evidence for that claim:
# the compiler, the packs selected, the BDD target if one is built, and for each
# selected pack the version of every upstream tree it compiles against, read from
# the version header in the tree rather than from an image tag or a Dockerfile.
# Where the tree is a git checkout its tag and commit are recorded beside the
# header's version, so a tree whose header lags its tag shows both.
#
# The record is ${CMAKE_BINARY_DIR}/solidsyslog-versions.json. Every CI lane
# uploads it beside its JUnit results, scripts/junit_report_pages.py shows it per
# lane, and scripts/check_tested_versions.py holds each platform page's
# `<!-- tested-against: ... -->` marker to it.
#
#   solidsyslog_write_version_record(<bdd target or empty>)
#
# Top-level only: a consumer's configure records nothing.

# One upstream tree a pack compiles against: the cache variable that locates it,
# the header carrying its version relative to that root, and how the version is
# spelt in the header - STRING for one quoted macro, DOTTED for numeric macros
# (comma-separated here) joined with dots, HEX16 for one 0xMMMMmmmm macro, NONE
# where the tree states no version a reader would know and git alone identifies
# it. A pack with more than one tree lists
# each; a pack whose upstream is found by find_package rather than a tree is
# handled by name below.
set(SOLIDSYSLOG_UPSTREAM_TREES
    "MbedTls|SOLIDSYSLOG_MBEDTLS_DIR|include/mbedtls/build_info.h|STRING|MBEDTLS_VERSION_STRING"
    "WolfSsl|SOLIDSYSLOG_WOLFSSL_DIR|include/wolfssl/version.h|STRING|LIBWOLFSSL_VERSION_STRING"
    "WolfSsl|SOLIDSYSLOG_WOLFSSL_BASELINE_DIR|include/wolfssl/version.h|STRING|LIBWOLFSSL_VERSION_STRING"
    "WolfSsl|SOLIDSYSLOG_WOLFSSL_SOURCE_DIR|wolfssl/version.h|STRING|LIBWOLFSSL_VERSION_STRING"
    "LwipRaw|SOLIDSYSLOG_LWIP_PATH|src/include/lwip/init.h|DOTTED|LWIP_VERSION_MAJOR,LWIP_VERSION_MINOR,LWIP_VERSION_REVISION"
    "LwipSocket|SOLIDSYSLOG_LWIP_PATH|src/include/lwip/init.h|DOTTED|LWIP_VERSION_MAJOR,LWIP_VERSION_MINOR,LWIP_VERSION_REVISION"
    "PlusTcp|SOLIDSYSLOG_FREERTOS_PLUS_TCP_PATH|source/include/FreeRTOS_IP.h|STRING|ipFR_TCP_VERSION_NUMBER"
    "FreeRtos|SOLIDSYSLOG_FREERTOS_KERNEL_PATH|include/task.h|STRING|tskKERNEL_VERSION_NUMBER"
    "CmsisRtos|SOLIDSYSLOG_CMSIS_PATH||NONE|"
    "CmsisRtos|SOLIDSYSLOG_CMSIS_FREERTOS_PATH||NONE|"
    "FatFs|SOLIDSYSLOG_FATFS_PATH||NONE|"
    "PlusFat|SOLIDSYSLOG_FREERTOS_PLUS_FAT_PATH||NONE|"
    "LittleFs|SOLIDSYSLOG_LITTLEFS_PATH|lfs.h|HEX16|LFS_VERSION"
)

# The value of one object-like macro in a header, as written, or empty.
function(solidsyslog_header_macro out header macro)
    set(value "")
    if(EXISTS "${header}")
        file(STRINGS "${header}" lines REGEX "^[ \t]*#[ \t]*define[ \t]+${macro}[ \t(]")
        foreach(line IN LISTS lines)
            if(line MATCHES "#[ \t]*define[ \t]+${macro}[ \t]+\"([^\"]*)\"")
                set(value "${CMAKE_MATCH_1}")
            elseif(line MATCHES "#[ \t]*define[ \t]+${macro}[ \t]+(0x[0-9A-Fa-f]+|[0-9]+)")
                set(value "${CMAKE_MATCH_1}")
            endif()
        endforeach()
    endif()
    set(${out} "${value}" PARENT_SCOPE)
endfunction()

# The version a tree's header states, spelt as the pages spell it, or empty.
function(solidsyslog_header_version out root header shape macros)
    set(version "")
    if(shape STREQUAL "STRING")
        solidsyslog_header_macro(version "${root}/${header}" "${macros}")
    elseif(shape STREQUAL "DOTTED")
        set(parts "")
        string(REPLACE "," ";" macros "${macros}")
        foreach(macro IN LISTS macros)
            solidsyslog_header_macro(part "${root}/${header}" "${macro}")
            if(part STREQUAL "")
                set(parts "")
                break()
            endif()
            list(APPEND parts "${part}")
        endforeach()
        string(JOIN "." version ${parts})
    elseif(shape STREQUAL "HEX16")
        solidsyslog_header_macro(packed "${root}/${header}" "${macros}")
        if(NOT packed STREQUAL "")
            math(EXPR major "${packed} >> 16")
            math(EXPR minor "${packed} & 65535")
            set(version "${major}.${minor}")
        endif()
    endif()
    set(${out} "${version}" PARENT_SCOPE)
endfunction()

# The tag and commit of a tree that is a git checkout, or empty. A tree that is
# not one, or a git that cannot read it, leaves both empty rather than failing
# the configure: the record is evidence, not a gate.
function(solidsyslog_tree_revision out_tag out_commit root)
    set(tag "")
    set(commit "")
    if(GIT_EXECUTABLE AND EXISTS "${root}/.git")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${root}" describe --tags --exact-match
            OUTPUT_VARIABLE tag OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE tag_result ERROR_QUIET)
        if(NOT tag_result EQUAL 0)
            set(tag "")
        endif()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${root}" rev-parse HEAD
            OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE commit_result ERROR_QUIET)
        if(NOT commit_result EQUAL 0)
            set(commit "")
        endif()
    endif()
    set(${out_tag} "${tag}" PARENT_SCOPE)
    set(${out_commit} "${commit}" PARENT_SCOPE)
endfunction()

function(solidsyslog_json_string out value)
    string(REPLACE "\\" "\\\\" value "${value}")
    string(REPLACE "\"" "\\\"" value "${value}")
    set(${out} "\"${value}\"" PARENT_SCOPE)
endfunction()

# A JSON array of strings from a CMake list.
function(solidsyslog_json_strings out values)
    set(items "")
    foreach(value IN LISTS values)
        solidsyslog_json_string(item "${value}")
        list(APPEND items "${item}")
    endforeach()
    string(JOIN ", " joined ${items})
    set(${out} "[${joined}]" PARENT_SCOPE)
endfunction()

# One upstream entry as a JSON object, keys with empty values left out.
function(solidsyslog_json_upstream out tree path version tag commit)
    set(members "")
    foreach(key IN ITEMS tree path version tag commit)
        if(NOT "${${key}}" STREQUAL "")
            solidsyslog_json_string(value "${${key}}")
            list(APPEND members "\"${key}\": ${value}")
        endif()
    endforeach()
    string(JOIN ", " joined ${members})
    set(${out} "{${joined}}" PARENT_SCOPE)
endfunction()

function(solidsyslog_write_version_record bdd_target)
    find_package(Git QUIET)

    set(packs "")
    foreach(row IN LISTS SOLIDSYSLOG_PLATFORM_REGISTRY)
        solidsyslog_read_platform("${row}")
        if(${platform_option})
            list(APPEND packs "${platform_token}")
        endif()
    endforeach()

    set(upstream_members "")
    foreach(pack IN LISTS packs)
        set(entries "")
        foreach(row IN LISTS SOLIDSYSLOG_UPSTREAM_TREES)
            string(REPLACE "|" ";" fields "${row}")
            list(GET fields 0 row_pack)
            if(NOT row_pack STREQUAL pack)
                continue()
            endif()
            list(GET fields 1 variable)
            list(GET fields 2 header)
            list(GET fields 3 shape)
            list(GET fields 4 macros)
            set(root "${${variable}}")
            if(root STREQUAL "")
                continue()
            endif()
            solidsyslog_header_version(version "${root}" "${header}" "${shape}" "${macros}")
            solidsyslog_tree_revision(tag commit "${root}")
            string(REPLACE "SOLIDSYSLOG_" "" tree "${variable}")
            solidsyslog_json_upstream(entry "${tree}" "${root}" "${version}" "${tag}" "${commit}")
            list(APPEND entries "${entry}")
        endforeach()
        if(pack STREQUAL "OpenSsl" AND OPENSSL_VERSION)
            solidsyslog_json_upstream(entry "OpenSSL" "" "${OPENSSL_VERSION}" "" "")
            list(APPEND entries "${entry}")
        endif()
        if(entries)
            string(JOIN ", " joined ${entries})
            list(APPEND upstream_members "\"${pack}\": [${joined}]")
        endif()
    endforeach()
    string(JOIN ",\n    " upstreams ${upstream_members})

    solidsyslog_json_string(compiler_id "${CMAKE_C_COMPILER_ID}")
    solidsyslog_json_string(compiler_version "${CMAKE_C_COMPILER_VERSION}")
    get_filename_component(compiler_name "${CMAKE_C_COMPILER}" NAME)
    solidsyslog_json_string(compiler_executable "${compiler_name}")
    solidsyslog_json_string(system_name "${CMAKE_SYSTEM_NAME}")
    solidsyslog_json_string(system_processor "${CMAKE_SYSTEM_PROCESSOR}")
    if(CMAKE_CROSSCOMPILING)
        set(cross "true")
    else()
        set(cross "false")
    endif()
    solidsyslog_json_strings(pack_list "${packs}")
    if(bdd_target STREQUAL "")
        set(target_json "null")
    else()
        solidsyslog_json_string(target_json "${bdd_target}")
    endif()

    file(WRITE "${CMAKE_BINARY_DIR}/solidsyslog-versions.json"
"{
  \"compiler\": {\"id\": ${compiler_id}, \"version\": ${compiler_version}, \"executable\": ${compiler_executable}},
  \"system\": {\"name\": ${system_name}, \"processor\": ${system_processor}, \"crossCompiling\": ${cross}},
  \"packs\": ${pack_list},
  \"bddTarget\": ${target_json},
  \"upstreams\": {
    ${upstreams}
  }
}
")
endfunction()
