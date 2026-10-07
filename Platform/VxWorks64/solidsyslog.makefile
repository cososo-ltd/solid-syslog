# SolidSyslog glue for a VxWorks 6.4 Workbench VIP. A DKM uses
# solidsyslog-dkm.makefile instead.
#
# Copy this file into the project directory: the project's generated Makefile
# includes every *.makefile it finds there. Point SOLIDSYSLOG_DIR at the
# SolidSyslog checkout, in the environment or on the line below.
#
# Before the project's own build, it builds libsolidsyslog.a through
# solidsyslog-vxworks64.mk with the project's compiler, archiver and target
# flags, and gives the project the library and its headers through
# ADDED_INCLUDES, ADDED_LIBPATH and ADDED_LIBS.
#
#   SOLIDSYSLOG_DIR            the SolidSyslog checkout (required)
#   SOLIDSYSLOG_PLATFORMS      platforms to build beside Core (default: none)
#   SOLIDSYSLOG_BUILD_DIR      where the library is built
#                              (default: solidsyslog in the project directory)
#   SOLIDSYSLOG_TARGET_CFLAGS  the flags the library is compiled with
#                              (default: the project's CFLAGS less its dialect)
#
# The generated Makefile defines CFLAGS and the toolchain after including this
# file, so everything here that reads them is recursively expanded - assigned
# with = or ?=, never :=.
#
# The library is built under external_build, which the project builds before its
# own targets. Under make -j that order is not guaranteed, because the
# project's targets do not depend on external_build.

# SOLIDSYSLOG_DIR ?= C:/path/to/solid-syslog

ifndef SOLIDSYSLOG_DIR
$(error SOLIDSYSLOG_DIR is not set - point it at the SolidSyslog checkout)
endif

SOLIDSYSLOG_PLATFORMS     ?=
SOLIDSYSLOG_BUILD_DIR     ?= $(PRJ_DIR)/solidsyslog
SOLIDSYSLOG_TARGET_CFLAGS ?= $(filter-out -Xansi -ansi -std=%,$(CFLAGS))

# Passed through the environment rather than the command line, so that no
# quoting in the project's flags has to survive a shell.
export SOLIDSYSLOG_TARGET_CFLAGS

export ADDED_INCLUDES += -I$(SOLIDSYSLOG_DIR)/Core/Interface \
	$(foreach p,$(SOLIDSYSLOG_PLATFORMS),-I$(SOLIDSYSLOG_DIR)/Platform/$(p)/Interface)
export ADDED_LIBPATH  += -L$(SOLIDSYSLOG_BUILD_DIR)
export ADDED_LIBS     += -lsolidsyslog

.PHONY: solidsyslog_library

external_build :: solidsyslog_library

solidsyslog_library:
	$(MAKE) -f $(SOLIDSYSLOG_DIR)/Platform/VxWorks64/solidsyslog-vxworks64.mk \
		CC="$(CC)" AR="$(AR)" TOOL_FAMILY="$(TOOL_FAMILY)" \
		SOLIDSYSLOG_BUILD_DIR="$(SOLIDSYSLOG_BUILD_DIR)" \
		SOLIDSYSLOG_PLATFORMS="$(SOLIDSYSLOG_PLATFORMS)"
