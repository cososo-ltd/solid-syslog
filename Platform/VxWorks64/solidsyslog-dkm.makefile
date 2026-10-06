# SolidSyslog glue for a VxWorks 6.4 Workbench DKM project.
#
# Include this from a managed-build extension makefile in the DKM project.
# It builds libsolidsyslog.a with the DKM's compiler, CPU and debug settings,
# then exports the library and its public headers through Workbench's ADDED_*
# extension variables.
#
# The library is built under external_build, which the project builds before its
# own targets. Under make -j that order is not guaranteed, because the
# project's targets do not depend on external_build.

SOLIDSYSLOG_VXWORKS64_DKM_MAKEFILE := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
SOLIDSYSLOG_DIR ?= $(dir $(SOLIDSYSLOG_VXWORKS64_DKM_MAKEFILE))../..

SOLIDSYSLOG_PLATFORMS ?= VxWorks64

# The library is built per build spec and mode, under the project. Workbench may
# define these after including this file, so they are read only when a recipe
# runs. Without them the default would name a directory at the root of the
# drive, which clean would remove; an explicit SOLIDSYSLOG_BUILD_DIR needs none.
SOLIDSYSLOG_DKM_DEFAULT_BUILD_DIR = $(PRJ_ROOT_DIR)/solidsyslog/$(BUILD_SPEC)/$(MODE_DIR)
SOLIDSYSLOG_BUILD_DIR ?= $(SOLIDSYSLOG_DKM_DEFAULT_BUILD_DIR)
SOLIDSYSLOG_DKM_WORKBENCH_SET = $(and $(strip $(PRJ_ROOT_DIR)),$(strip $(BUILD_SPEC)),$(strip $(MODE_DIR)))
SOLIDSYSLOG_DKM_BUILD_DIR_UNSAFE = $(if $(filter $(SOLIDSYSLOG_DKM_DEFAULT_BUILD_DIR),$(SOLIDSYSLOG_BUILD_DIR)),$(if $(SOLIDSYSLOG_DKM_WORKBENCH_SET),,unsafe))

# Workbench publishes the target, debug, include and preprocessor settings,
# but its generated VxWorks 6.4 DKM Makefile does not publish the compiler or
# archiver command: it spells them directly in each recipe.  The small product
# adapter which includes this file therefore supplies these three variables.
# Keeping the tool invocation there makes this reusable by a GNU DKM too.
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

# Passed through the environment rather than the command line, so that no
# quoting in the project's flags has to survive a shell.
export SOLIDSYSLOG_TARGET_CFLAGS SOLIDSYSLOG_BUILD_DIR SOLIDSYSLOG_PLATFORMS

.PHONY: solidsyslog_build_dir_check solidsyslog_library

external_build :: solidsyslog_library

solidsyslog_build_dir_check:
	$(if $(SOLIDSYSLOG_DKM_BUILD_DIR_UNSAFE),$(error PRJ_ROOT_DIR and BUILD_SPEC and MODE_DIR are not all set - include this from a Workbench DKM build or set SOLIDSYSLOG_BUILD_DIR))

# The lower-level makefile uses the conventional CC and AR names.  The DKM
# adapter supplies their values under SolidSyslog-specific names so they do not
# leak into the product project; map just those command names for the sub-make.
# Quoted, so a command that carries its own arguments arrives whole.
solidsyslog_library: solidsyslog_build_dir_check
	$(MAKE) -f $(SOLIDSYSLOG_DIR)/Platform/VxWorks64/solidsyslog-vxworks64.mk \
		CC="$(SOLIDSYSLOG_CC)" AR="$(SOLIDSYSLOG_AR)" TOOL_FAMILY="$(TOOL_FAMILY)"

external_clean :: solidsyslog_build_dir_check
	$(MAKE) -f $(SOLIDSYSLOG_DIR)/Platform/VxWorks64/solidsyslog-vxworks64.mk \
		CC="$(SOLIDSYSLOG_CC)" AR="$(SOLIDSYSLOG_AR)" TOOL_FAMILY="$(TOOL_FAMILY)" clean
