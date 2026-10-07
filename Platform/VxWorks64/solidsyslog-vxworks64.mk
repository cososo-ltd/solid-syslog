# SolidSyslog library build for VxWorks 6.4 kernel projects.
#
# Builds Core, plus any platforms named in SOLIDSYSLOG_PLATFORMS, into one static
# library with the Wind River toolchain a project already uses. It names no CPU:
# everything about the target arrives from the project that runs it - normally
# through solidsyslog.makefile for a VIP, or solidsyslog-dkm.makefile for a DKM. What
# it adds is C99: the dialect, because the flags a project generates select an
# older one, and the C99 library headers the kernel header tree lacks, from
# Compat/ beside this file.
#
#   make -f solidsyslog-vxworks64.mk CC=dcc AR=dar TOOL_FAMILY=diab \
#        SOLIDSYSLOG_TARGET_CFLAGS="<the project's target flags>" \
#        SOLIDSYSLOG_BUILD_DIR=<output directory>
#
#   CC, AR                     the project's compiler and archiver
#   TOOL_FAMILY                diab or gnu; selects the dialect
#   SOLIDSYSLOG_TARGET_CFLAGS  the project's architecture, define and include
#                              flags, with its own dialect flag removed
#   SOLIDSYSLOG_BUILD_DIR      objects and libsolidsyslog.a are written here
#   SOLIDSYSLOG_PLATFORMS      optional; as for solidsyslog.mk
#
# Targets: all (the default), clean, print-config.

SOLIDSYSLOG_VXWORKS64_MK := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
SOLIDSYSLOG_DIR ?= $(dir $(SOLIDSYSLOG_VXWORKS64_MK))../..

# Make supplies cc and ar on its own; neither is a Wind River tool.
ifeq ($(origin CC),default)
$(error CC is not set - pass the project's compiler)
endif
ifeq ($(origin AR),default)
$(error AR is not set - pass the project's archiver)
endif
ifndef SOLIDSYSLOG_TARGET_CFLAGS
$(error SOLIDSYSLOG_TARGET_CFLAGS is not set - pass the project's target flags)
endif
ifndef SOLIDSYSLOG_BUILD_DIR
$(error SOLIDSYSLOG_BUILD_DIR is not set - name a directory for the build output)
endif

# -ei4188 turns off "enumerated type mixed with another type". In C an
# enumeration constant has type int (C99 6.4.4.3), so Diab raises it wherever an
# enum is given one of its own constants - the ordinary use of every enum in the
# library. MISRA's essential type model already treats such a constant as its
# enum's type.
#
# The platform sources include the Wind River headers, which the C99 front end
# finds two faults in that the project's own dialect does not report: -ei4301
# for a typedef repeated with the same type (size_t), -ei4381 for an extra ";"
# after a declaration. Core includes no Wind River header and keeps both.
ifeq ($(TOOL_FAMILY),diab)
SOLIDSYSLOG_DIALECT_CFLAGS        := -Xdialect-c99 -ei4188
SOLIDSYSLOG_PLATFORM_EXTRA_CFLAGS := -ei4301,4381
SOLIDSYSLOG_DEPEND_CFLAGS         := -Xmake-dependency=0xd
endif
ifeq ($(TOOL_FAMILY),gnu)
SOLIDSYSLOG_DIALECT_CFLAGS := -std=c99
SOLIDSYSLOG_DEPEND_CFLAGS  := -MMD
endif
ifndef SOLIDSYSLOG_DIALECT_CFLAGS
$(error TOOL_FAMILY is '$(TOOL_FAMILY)' - expected diab or gnu)
endif

include $(SOLIDSYSLOG_DIR)/solidsyslog.mk

# Put the C99 compatibility headers before the project's include directories:
# the VxWorks 6.4 DKM header path otherwise selects an incomplete stdint.h.
# This ordering is private to the library; do not export Compat to DKM sources,
# whose existing headers may provide their own incompatible integer typedefs.
SOLIDSYSLOG_C99_INCLUDES := -I$(SOLIDSYSLOG_DIR)/Platform/VxWorks64/Compat

SOLIDSYSLOG_LIB := $(SOLIDSYSLOG_BUILD_DIR)/libsolidsyslog.a

# Objects share one directory, which every source name being unique allows, so
# no rule has to match a Windows drive letter inside a path pattern.
SOLIDSYSLOG_CORE_OBJS     := $(addprefix $(SOLIDSYSLOG_BUILD_DIR)/, \
	$(notdir $(SOLIDSYSLOG_CORE_SRCS:.c=.o)))
SOLIDSYSLOG_PLATFORM_OBJS := $(addprefix $(SOLIDSYSLOG_BUILD_DIR)/, \
	$(notdir $(SOLIDSYSLOG_PLATFORM_SRCS:.c=.o)))
SOLIDSYSLOG_OBJS          := $(SOLIDSYSLOG_CORE_OBJS) $(SOLIDSYSLOG_PLATFORM_OBJS)

vpath %.c $(sort $(dir $(SOLIDSYSLOG_SRCS)))

# Core sees the library's own headers alone, as solidsyslog.mk intends.
$(SOLIDSYSLOG_CORE_OBJS):     SOLIDSYSLOG_OBJ_INCLUDES := $(SOLIDSYSLOG_CORE_INCLUDES)
$(SOLIDSYSLOG_PLATFORM_OBJS): SOLIDSYSLOG_OBJ_INCLUDES := $(SOLIDSYSLOG_INCLUDES)
$(SOLIDSYSLOG_PLATFORM_OBJS): SOLIDSYSLOG_OBJ_CFLAGS   := $(SOLIDSYSLOG_PLATFORM_EXTRA_CFLAGS)

.PHONY: all clean print-config

all: $(SOLIDSYSLOG_LIB)

$(SOLIDSYSLOG_LIB): $(SOLIDSYSLOG_OBJS)
	$(AR) crs $@ $(SOLIDSYSLOG_OBJS)

$(SOLIDSYSLOG_BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(SOLIDSYSLOG_C99_INCLUDES) $(SOLIDSYSLOG_TARGET_CFLAGS) $(SOLIDSYSLOG_DIALECT_CFLAGS) \
		$(SOLIDSYSLOG_OBJ_CFLAGS) $(SOLIDSYSLOG_DEPEND_CFLAGS) $(SOLIDSYSLOG_OBJ_INCLUDES) \
		-c $< -o $@

clean:
	rm -rf $(SOLIDSYSLOG_BUILD_DIR)

print-config:
	@echo CC=$(CC)
	@echo AR=$(AR)
	@echo TOOL_FAMILY=$(TOOL_FAMILY)
	@echo SOLIDSYSLOG_TARGET_CFLAGS=$(SOLIDSYSLOG_TARGET_CFLAGS)
	@echo SOLIDSYSLOG_DIALECT_CFLAGS=$(SOLIDSYSLOG_DIALECT_CFLAGS)
	@echo SOLIDSYSLOG_PLATFORM_EXTRA_CFLAGS=$(SOLIDSYSLOG_PLATFORM_EXTRA_CFLAGS)
	@echo SOLIDSYSLOG_C99_INCLUDES=$(SOLIDSYSLOG_C99_INCLUDES)
	@echo SOLIDSYSLOG_PLATFORMS=$(SOLIDSYSLOG_PLATFORMS)
	@echo SOLIDSYSLOG_LIB=$(SOLIDSYSLOG_LIB)

-include $(SOLIDSYSLOG_OBJS:.o=.d)
