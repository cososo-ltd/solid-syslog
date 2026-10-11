# SolidSyslog glue for a VxWorks 6.4 Workbench DKM project.
#
# Include this from a managed-build extension makefile in the DKM project.
# It builds libsolidsyslog.a with the DKM's compiler, CPU and debug settings,
# then exports the library and its public headers through Workbench's ADDED_*
# extension variables.
#
# The library is built under external_build, and the DKM's link targets,
# PROJECT_TARGETS, depend on the archive itself. The sub-make runs on every
# build and rewrites the archive only when a library source or header has
# changed, so a DKM relinks when the library changes and not otherwise.
#
# ADDED_CFLAGS names the library's solidsyslog-config.h in
# SOLIDSYSLOG_USER_TUNABLES_FILE, so the DKM's sources see the tunables the
# library was built with: those in SOLIDSYSLOG_TUNABLES_FILE, if it names a
# header, and the defaults otherwise. The library rewrites that header only when
# its configuration changes, and a DKM source that reads a tunable depends on
# it, so the source recompiles when the library's configuration changes.
# Under make -j the first build can compile such a source before the header
# exists; build again and it is found.
# SOLIDSYSLOG_CONFIG_HEADER names the header, for a rule of the including
# makefile's own that compiles a source against it.

SOLIDSYSLOG_VXWORKS64_DKM_MAKEFILE := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
SOLIDSYSLOG_DIR ?= $(dir $(SOLIDSYSLOG_VXWORKS64_DKM_MAKEFILE))../..

SOLIDSYSLOG_PLATFORMS ?= VxWorks64
SOLIDSYSLOG_TUNABLES_FILE ?=

# The library is built per build spec and mode, under the project. Workbench's
# generated Makefile defines these before including this file, as the archive
# rule below needs: a rule's target and prerequisites are read with the file.
# Without them the default would name a directory at the root of the drive,
# which clean would remove; an explicit SOLIDSYSLOG_BUILD_DIR needs none.
SOLIDSYSLOG_DKM_DEFAULT_BUILD_DIR = $(PRJ_ROOT_DIR)/solidsyslog/$(BUILD_SPEC)/$(MODE_DIR)
SOLIDSYSLOG_BUILD_DIR ?= $(SOLIDSYSLOG_DKM_DEFAULT_BUILD_DIR)
SOLIDSYSLOG_LIB = $(SOLIDSYSLOG_BUILD_DIR)/libsolidsyslog.a
SOLIDSYSLOG_CONFIG_HEADER = $(SOLIDSYSLOG_BUILD_DIR)/solidsyslog-config.h
# Wind River VxWorks 6.4 ships GNU Make 3.80, which has no $(and ...) function.
SOLIDSYSLOG_DKM_WORKBENCH_SET = $(if $(strip $(PRJ_ROOT_DIR)),$(if $(strip $(BUILD_SPEC)),$(if $(strip $(MODE_DIR)),set)))
SOLIDSYSLOG_DKM_BUILD_DIR_UNSAFE = $(if $(filter $(SOLIDSYSLOG_DKM_DEFAULT_BUILD_DIR),$(SOLIDSYSLOG_BUILD_DIR)),$(if $(SOLIDSYSLOG_DKM_WORKBENCH_SET),,unsafe))

# The generated VxWorks 6.4 DKM Makefile spells the compiler and archiver in
# each recipe rather than publishing them, so the makefile that includes this
# one supplies these variables.
ifeq ($(origin SOLIDSYSLOG_CC),undefined)
$(error SOLIDSYSLOG_CC is not set - pass the DKM compiler command)
endif
ifeq ($(origin SOLIDSYSLOG_AR),undefined)
$(error SOLIDSYSLOG_AR is not set - pass the DKM archiver command)
endif
ifeq ($(origin SOLIDSYSLOG_TARGET_CFLAGS),undefined)
$(error SOLIDSYSLOG_TARGET_CFLAGS is not set - pass the DKM target flags)
endif

export ADDED_INCLUDES += -I$(SOLIDSYSLOG_DIR)/Core/Interface \
	$(foreach p,$(SOLIDSYSLOG_PLATFORMS),-I$(SOLIDSYSLOG_DIR)/Platform/$(p)/Interface)
export ADDED_LIBPATH += -L$(SOLIDSYSLOG_BUILD_DIR)
export ADDED_LIBS += -lsolidsyslog
export ADDED_CFLAGS += -DSOLIDSYSLOG_USER_TUNABLES_FILE=\"$(SOLIDSYSLOG_CONFIG_HEADER)\"

# Passed through the environment rather than the command line, so that no
# quoting in the project's flags has to survive a shell.
export SOLIDSYSLOG_TARGET_CFLAGS SOLIDSYSLOG_BUILD_DIR SOLIDSYSLOG_PLATFORMS SOLIDSYSLOG_TUNABLES_FILE

.PHONY: solidsyslog_config_check solidsyslog_library

external_build :: solidsyslog_library

$(PROJECT_TARGETS): $(SOLIDSYSLOG_LIB)

# The library's build writes the header. This rule orders a DKM source that
# depends on it after that build, and its recipe makes make read the header's
# timestamp again once the build has run.
$(SOLIDSYSLOG_CONFIG_HEADER): $(SOLIDSYSLOG_LIB)
	@:

# Defined but empty passes the checks above, so each value is checked again here,
# when it is used and whatever defined it has been read.
solidsyslog_config_check:
	$(if $(strip $(SOLIDSYSLOG_CC)),,$(error SOLIDSYSLOG_CC is empty - pass the DKM compiler command))
	$(if $(strip $(SOLIDSYSLOG_AR)),,$(error SOLIDSYSLOG_AR is empty - pass the DKM archiver command))
	$(if $(strip $(SOLIDSYSLOG_TARGET_CFLAGS)),,$(error SOLIDSYSLOG_TARGET_CFLAGS is empty - pass the DKM target flags))
	$(if $(strip $(SOLIDSYSLOG_BUILD_DIR)),,$(error SOLIDSYSLOG_BUILD_DIR is empty - leave it unset for the default or name a directory))
	$(if $(SOLIDSYSLOG_DKM_BUILD_DIR_UNSAFE),$(error PRJ_ROOT_DIR and BUILD_SPEC and MODE_DIR are not all set - include this from a Workbench DKM build or set SOLIDSYSLOG_BUILD_DIR))

# Workbench's external-build entry point.
solidsyslog_library: $(SOLIDSYSLOG_LIB)

# The phony config check makes the sub-make run on every build; its dependency
# files decide whether the archive actually changes. The archive is a real file,
# never phony and never touched here, so its timestamp is the library's own.
#
# The sub-make takes CC and AR; they are passed under SolidSyslog names so they
# do not reach the DKM project. Quoted, so a command that carries its own
# arguments arrives whole.
$(SOLIDSYSLOG_LIB): solidsyslog_config_check
	$(MAKE) -f $(SOLIDSYSLOG_DIR)/Platform/VxWorks64/solidsyslog-vxworks64.mk \
		CC="$(SOLIDSYSLOG_CC)" AR="$(SOLIDSYSLOG_AR)" TOOL_FAMILY="$(TOOL_FAMILY)"

external_clean :: solidsyslog_config_check
	$(MAKE) -f $(SOLIDSYSLOG_DIR)/Platform/VxWorks64/solidsyslog-vxworks64.mk \
		CC="$(SOLIDSYSLOG_CC)" AR="$(SOLIDSYSLOG_AR)" TOOL_FAMILY="$(TOOL_FAMILY)" clean
