# Integration manifest generator (S30.03).
#
# Emits the file / include-dir / -D manifest a NON-CMake integrator (IAR / Keil /
# MPLAB / CCS / hand Makefile) needs to compile SolidSyslog into their own
# project. The .c file lists are read straight from the build targets — the Core
# library's SOURCES and each selected upstream platform's INTERFACE_SOURCES
# (populated in S30.02) — so the manifest can never drift from what the platforms
# actually ship.
#
# Selection: SOLIDSYSLOG_MANIFEST_PLATFORMS is a ;-list of platform tokens from
# SOLIDSYSLOG_PLATFORM_REGISTRY, e.g. "LwipRaw;MbedTls;FreeRtos;Atomics".
# Empty (the default) means "every platform this configure selected". Core is
# always included.
#
# The registry is the only platform vocabulary in the repo (S30.06) and this
# generator holds no list of its own (S33.03). Its `kind` column is already the
# distinction the manifest has to present:
#
#   upstream — a SolidSyslog::<Token> target carrying its own INTERFACE_SOURCES.
#   probe    — compiled into the Core target by target_sources(SolidSyslog
#              PRIVATE ...). A non-CMake build has no libSolidSyslog.a to get
#              those from, so they must be listed too; leaving them out is what
#              silently cost an integrator their atomic counter.
#
# Output: written to ${SolidSyslog_BINARY_DIR}/solidsyslog-manifest.txt at configure
# time, and printed by `cmake --build <dir> --target manifest`. If
# SOLIDSYSLOG_MANIFEST_OUTPUT is set it is ALSO written there (used to refresh the
# committed docs/generated sample, which CI diff-checks for drift).

set(SOLIDSYSLOG_MANIFEST_PLATFORMS "" CACHE STRING
    "Platforms to include in the generated integration manifest (;-list of \
tokens from SOLIDSYSLOG_PLATFORM_REGISTRY). Empty = every platform this \
configure selected.")
set(SOLIDSYSLOG_MANIFEST_OUTPUT "" CACHE FILEPATH
    "Optional extra path to also write the generated manifest to (e.g. the \
committed docs/generated sample).")
set(SOLIDSYSLOG_MANIFEST_SCOPE "all" CACHE STRING
    "Which part of the manifest to emit: all, core (Core only) or platform \
(the selected platforms only). core + one platform per platform you picked is \
the same content as all, split so no combination has to be named.")
set_property(CACHE SOLIDSYSLOG_MANIFEST_SCOPE PROPERTY STRINGS all core platform)

# Per-platform manifest knowledge, keyed by registry token: the integrator-supplied
# config header(s) the platform requires, and any language standard it raises above
# the floor. Stable knowledge, which is why it lives here and not in the registry —
# the registry says what the build does. The volatile .c lists come from the targets.
set(_SOLIDSYSLOG_MANIFEST_CFG_FreeRtos           "FreeRTOSConfig.h")
set(_SOLIDSYSLOG_MANIFEST_CFG_PlusTcp            "FreeRTOSConfig.h, FreeRTOSIPConfig.h")
set(_SOLIDSYSLOG_MANIFEST_CFG_LwipRaw            "lwipopts.h")
set(_SOLIDSYSLOG_MANIFEST_CFG_MbedTls            "mbedtls_config.h")
set(_SOLIDSYSLOG_MANIFEST_CFG_FatFs              "ffconf.h")
set(_SOLIDSYSLOG_MANIFEST_CFG_PlusFat            "FreeRTOSFATConfig.h")

set(_SOLIDSYSLOG_MANIFEST_LANGUAGE_FLOOR         "C99")
set(_SOLIDSYSLOG_MANIFEST_LANG_Atomics           "C11")

# Make an absolute path repo-relative for display; pass others through unchanged.
function(_solidsyslog_manifest_relpath OUT_VAR PATH)
    set(_p "${PATH}")
    if(IS_ABSOLUTE "${_p}")
        file(RELATIVE_PATH _p "${SolidSyslog_SOURCE_DIR}" "${_p}")
    endif()
    set(${OUT_VAR} "${_p}" PARENT_SCOPE)
endfunction()

# Append the .c sources of an upstream platform's INTERFACE target (repo-relative)
# to OUT_VAR.
function(_solidsyslog_manifest_upstream_sources OUT_VAR TARGET)
    set(_lines "")
    get_target_property(_srcs "${TARGET}" INTERFACE_SOURCES)
    if(_srcs)
        foreach(_s ${_srcs})
            _solidsyslog_manifest_relpath(_rel "${_s}")
            string(APPEND _lines "${_rel}\n")
        endforeach()
    endif()
    set(${OUT_VAR} "${_lines}" PARENT_SCOPE)
endfunction()

function(solidsyslog_generate_manifest)
    # Flatten the registry into per-token lookups. Registry order is the manifest's
    # display order throughout, so the output is a function of the selection alone
    # and not of the order the selection was typed — the committed sample is
    # diff-checked in CI.
    set(_known "")
    foreach(_row IN LISTS SOLIDSYSLOG_PLATFORM_REGISTRY)
        solidsyslog_read_platform("${_row}")
        list(APPEND _known ${platform_token})
        set(_kind_${platform_token} "${platform_kind}")
        set(_dir_${platform_token} "${platform_directory}")
        set(_on_${platform_token} ${${platform_option}})
    endforeach()

    set(_requested ${_known})
    if(SOLIDSYSLOG_MANIFEST_PLATFORMS)
        foreach(_token IN LISTS SOLIDSYSLOG_MANIFEST_PLATFORMS)
            if(NOT _token IN_LIST _known)
                message(FATAL_ERROR
                    "SOLIDSYSLOG_MANIFEST_PLATFORMS names an unknown platform "
                    "'${_token}'. Valid platforms: ${_known}.")
            endif()
        endforeach()
        set(_requested ${SOLIDSYSLOG_MANIFEST_PLATFORMS})
    endif()

    # A platform can only be described if this configure actually selected it:
    # an upstream one defines its target, a probe one compiles into Core. Naming
    # one that did not is worth saying out loud; the default selection just skips.
    set(_selected "")
    set(_selected_upstream "")
    set(_selected_probe "")
    foreach(_token IN LISTS _known)
        if(NOT _token IN_LIST _requested)
            continue()
        endif()
        if(_kind_${_token} STREQUAL "upstream")
            if(TARGET SolidSyslog::${_token})
                list(APPEND _selected_upstream ${_token})
            elseif(SOLIDSYSLOG_MANIFEST_PLATFORMS)
                message(WARNING
                    "Manifest: SolidSyslog::${_token} is not a defined target in "
                    "this configuration; skipping. (Is it in SOLIDSYSLOG_PLATFORMS?)")
            endif()
        elseif(_on_${_token})
            list(APPEND _selected_probe ${_token})
        elseif(SOLIDSYSLOG_MANIFEST_PLATFORMS)
            message(WARNING
                "Manifest: the ${_token} platform is not selected in this "
                "configuration; skipping. (Is it in SOLIDSYSLOG_PLATFORMS, and "
                "does its availability probe pass on this toolchain?)")
        endif()
    endforeach()
    set(_selected ${_selected_upstream} ${_selected_probe})

    if(NOT SOLIDSYSLOG_MANIFEST_SCOPE MATCHES "^(all|core|platform)$")
        message(FATAL_ERROR
            "SOLIDSYSLOG_MANIFEST_SCOPE must be all, core or platform "
            "(got: '${SOLIDSYSLOG_MANIFEST_SCOPE}').")
    endif()
    set(_scope "${SOLIDSYSLOG_MANIFEST_SCOPE}")
    if(_scope STREQUAL "core")
        set(_selected "")
        set(_selected_upstream "")
        set(_selected_probe "")
    endif()

    string(REPLACE ";" ", " _selected_display "${_selected}")
    if(NOT _selected_display)
        set(_selected_display "(none — Core only)")
    endif()

    # --- Header ---------------------------------------------------------------
    set(_m "# SolidSyslog integration manifest — GENERATED by CMake (S30.03).\n")
    string(APPEND _m "# Do not edit by hand. Regenerate with: cmake --build <build-dir> --target manifest\n")
    string(APPEND _m "# Scope: ${_scope}\n")
    string(APPEND _m "# Selected platforms: ${_selected_display}\n")
    string(APPEND _m "#\n")
    string(APPEND _m "# A non-CMake integrator (IAR / Keil / MPLAB / CCS / Make) compiles the .c\n")
    string(APPEND _m "# files below, with the include dirs on the compiler path, against their own\n")
    string(APPEND _m "# config headers. See docs/getting-started.md for the walkthrough.\n")
    if(_scope STREQUAL "core")
        string(APPEND _m "#\n")
        string(APPEND _m "# This is the Core half, required by every integration. Add one platform\n")
        string(APPEND _m "# manifest for each platform you picked; there is no fixed combination.\n")
    elseif(_scope STREQUAL "platform")
        string(APPEND _m "#\n")
        string(APPEND _m "# This is one platform's half. It is not usable alone — combine it with the\n")
        string(APPEND _m "# Core manifest and with the other platforms you picked.\n")
    endif()
    string(APPEND _m "\n")

    # --- Source files ---------------------------------------------------------
    #
    # Core's SOURCES carries the probe platforms' sources too — they attach with
    # target_sources(SolidSyslog PRIVATE ...) from Platform/<X>/CMakeLists.txt and
    # arrive as absolute paths. Bucket them by the registry's directory column
    # instead of dropping them.
    foreach(_token IN LISTS _selected_probe)
        set(_probe_srcs_${_token} "")
    endforeach()

    set(_core_lines "")
    get_target_property(_core_srcs SolidSyslog SOURCES)
    foreach(_s ${_core_srcs})
        if(NOT IS_ABSOLUTE "${_s}")
            string(APPEND _core_lines "Core/Source/${_s}\n")
            continue()
        endif()
        _solidsyslog_manifest_relpath(_rel "${_s}")
        set(_owner "")
        foreach(_token IN LISTS _known)
            if(_rel MATCHES "^${_dir_${_token}}/")
                set(_owner ${_token})
                break()
            endif()
        endforeach()
        if(_owner)
            if(_owner IN_LIST _selected_probe)
                string(APPEND _probe_srcs_${_owner} "${_rel}\n")
            endif()
        elseif(_rel MATCHES "^Core/Source/")
            string(APPEND _core_lines "${_rel}\n")
        else()
            message(WARNING
                "Manifest: '${_rel}' is in the Core target but under neither "
                "Core/Source nor a registered platform directory, so the manifest "
                "cannot place it. Add its directory to SOLIDSYSLOG_PLATFORM_REGISTRY.")
        endif()
    endforeach()

    string(APPEND _m "## Source files (.c) — compile all of these\n\n")
    set(_needs_gap FALSE)
    if(NOT _scope STREQUAL "platform")
        string(APPEND _m "# Core (always required):\n")
        string(APPEND _m "${_core_lines}")
        set(_needs_gap TRUE)
    endif()

    foreach(_token IN LISTS _selected_upstream)
        if(_needs_gap)
            string(APPEND _m "\n")
        endif()
        set(_needs_gap TRUE)
        string(APPEND _m "# ${_token}:\n")
        _solidsyslog_manifest_upstream_sources(_upstream_srcs SolidSyslog::${_token})
        string(APPEND _m "${_upstream_srcs}")
    endforeach()

    if(_selected_probe)
        if(_needs_gap)
            string(APPEND _m "\n")
        endif()
        string(APPEND _m "# The platforms below are selected by a toolchain capability probe. A CMake\n")
        string(APPEND _m "# consumer gets them compiled into libSolidSyslog.a; every other build has no\n")
        string(APPEND _m "# such library, so compile them explicitly with everything else.\n")
        foreach(_token IN LISTS _selected_probe)
            string(APPEND _m "\n# ${_token}:\n")
            string(APPEND _m "${_probe_srcs_${_token}}")
        endforeach()
    endif()

    # --- Include directories --------------------------------------------------
    string(APPEND _m "\n## Include directories\n\n")
    set(_incdirs "")
    if(NOT _scope STREQUAL "platform")
        list(APPEND _incdirs "Core/Interface" "Core/Source")
    endif()
    foreach(_token IN LISTS _selected_upstream)
        get_target_property(_incs SolidSyslog::${_token} INTERFACE_INCLUDE_DIRECTORIES)
        if(_incs)
            foreach(_i ${_incs})
                _solidsyslog_manifest_relpath(_rel "${_i}")
                list(APPEND _incdirs "${_rel}")
            endforeach()
        endif()
    endforeach()
    if(_selected_probe)
        # A probe platform publishes its Interface on the Core target, the same
        # way it publishes its sources into Core's SOURCES.
        get_target_property(_core_incs SolidSyslog INTERFACE_INCLUDE_DIRECTORIES)
        foreach(_i ${_core_incs})
            _solidsyslog_manifest_relpath(_rel "${_i}")
            foreach(_token IN LISTS _selected_probe)
                if(_rel MATCHES "^${_dir_${_token}}/")
                    list(APPEND _incdirs "${_rel}")
                endif()
            endforeach()
        endforeach()
    endif()
    list(REMOVE_DUPLICATES _incdirs)
    foreach(_d ${_incdirs})
        string(APPEND _m "${_d}\n")
    endforeach()
    if(_selected_upstream)
        string(APPEND _m "# Plus, located in YOUR project: each upstream library's own include dirs\n")
        string(APPEND _m "# and the directory holding your config headers.\n")
    endif()

    # --- Required defines -----------------------------------------------------
    # Bodies are built before their heading so a scope with nothing to say omits
    # the section rather than emitting an empty one.
    set(_defines "")
    if(NOT _scope STREQUAL "platform")
        string(APPEND _defines "-DSOLIDSYSLOG_USER_TUNABLES_FILE=\"my_tunables.h\"   # your tunable overrides (optional)\n")
    endif()
    list(FIND _selected "LwipRaw" _has_lwip)
    if(NOT _has_lwip EQUAL -1)
        string(APPEND _defines "# lwIP: each adapter gates itself on the lwIP option it needs, so a source you\n")
        string(APPEND _defines "#       do not enable compiles to nothing. LWIP_DNS=1 enables the DNS resolver;\n")
        string(APPEND _defines "#       LWIP_UDP / LWIP_TCP the datagram / stream adapters.\n")
    endif()
    list(FIND _selected "MbedTls" _has_mbedtls)
    if(NOT _has_mbedtls EQUAL -1)
        string(APPEND _defines "# Mbed TLS: point MBEDTLS_USER_CONFIG_FILE (or MBEDTLS_CONFIG_FILE) at your mbedtls config,\n")
        string(APPEND _defines "#           and use the SAME config when building the mbedTLS library itself.\n")
    endif()
    if(_defines)
        string(APPEND _m "\n## Required defines\n\n${_defines}")
    endif()

    # --- Language -------------------------------------------------------------
    # The floor, and any platform that raises it. Which -std= flag spells that is
    # the integrator's business: anything at or above the floor is supported.
    set(_language "")
    if(NOT _scope STREQUAL "platform")
        string(APPEND _language "${_SOLIDSYSLOG_MANIFEST_LANGUAGE_FLOOR} or later.\n")
    endif()
    foreach(_token IN LISTS _selected)
        set(_lang "${_SOLIDSYSLOG_MANIFEST_LANG_${_token}}")
        if(_lang)
            string(APPEND _language "The ${_token} platform requires ${_lang} or later.\n")
        endif()
    endforeach()
    if(_language)
        string(APPEND _m "\n## Language\n\n${_language}")
    endif()

    # --- Config headers you supply --------------------------------------------
    set(_configs "")
    foreach(_token IN LISTS _selected)
        set(_cfg "${_SOLIDSYSLOG_MANIFEST_CFG_${_token}}")
        if(_cfg)
            string(APPEND _configs "${_cfg}   (${_token})\n")
        endif()
    endforeach()
    if(_configs)
        string(APPEND _m "\n## Integrator-supplied config headers\n\n${_configs}")
    endif()

    # --- Emit -----------------------------------------------------------------
    set(_out "${SolidSyslog_BINARY_DIR}/solidsyslog-manifest.txt")
    file(WRITE "${_out}" "${_m}")
    set(SOLIDSYSLOG_MANIFEST_FILE "${_out}" CACHE INTERNAL "Generated manifest path")
    if(SOLIDSYSLOG_MANIFEST_OUTPUT)
        file(WRITE "${SOLIDSYSLOG_MANIFEST_OUTPUT}" "${_m}")
        message(STATUS "Manifest also written to ${SOLIDSYSLOG_MANIFEST_OUTPUT}")
    endif()

    # `cmake --build <dir> --target manifest` prints the generated file.
    if(NOT TARGET manifest)
        add_custom_target(manifest
            COMMAND ${CMAKE_COMMAND} -DSOLIDSYSLOG_MANIFEST_FILE=${_out}
                    -P ${SolidSyslog_SOURCE_DIR}/cmake/PrintManifest.cmake
            VERBATIM
            COMMENT "SolidSyslog integration manifest (${_selected_display})")
    endif()
endfunction()
