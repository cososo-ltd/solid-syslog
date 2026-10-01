# SolidSyslog library build for VxWorks 6.4 kernel projects.
#
# Builds Core, plus any platforms named in SOLIDSYSLOG_PLATFORMS, into one static
# library with the Wind River toolchain a project already uses. It names no CPU:
# everything about the target arrives from the project that runs it - normally
# solidsyslog.makefile, which a VIP or DKM picks up from its own directory. The
# one thing added here is the C99 dialect, because the flags a project generates
# select an older one.
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

# _C99 is the Wind River headers' own switch for their C99 declarations; the
# dialect flag alone leaves it unset.
ifeq ($(TOOL_FAMILY),diab)
SOLIDSYSLOG_DIALECT_CFLAGS := -Xdialect-c99 -D_C99
SOLIDSYSLOG_DEPEND_CFLAGS  := -Xmake-dependency=0xd
endif
ifeq ($(TOOL_FAMILY),gnu)
SOLIDSYSLOG_DIALECT_CFLAGS := -std=c99 -D_C99
SOLIDSYSLOG_DEPEND_CFLAGS  := -MMD
endif
ifndef SOLIDSYSLOG_DIALECT_CFLAGS
$(error TOOL_FAMILY is '$(TOOL_FAMILY)' - expected diab or gnu)
endif

# The kernel header tree has no stdbool.h; the user-mode tree beside it does.
# It goes last, so every header the kernel tree has is still taken from there.
ifdef WIND_BASE
SOLIDSYSLOG_C99_INCLUDES ?= -I$(subst \,/,$(WIND_BASE))/target/usr/h
endif

include $(SOLIDSYSLOG_DIR)/solidsyslog.mk

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

.PHONY: all clean print-config

all: $(SOLIDSYSLOG_LIB)

$(SOLIDSYSLOG_LIB): $(SOLIDSYSLOG_OBJS)
	$(AR) crs $@ $(SOLIDSYSLOG_OBJS)

$(SOLIDSYSLOG_BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(SOLIDSYSLOG_TARGET_CFLAGS) $(SOLIDSYSLOG_DIALECT_CFLAGS) \
		$(SOLIDSYSLOG_DEPEND_CFLAGS) $(SOLIDSYSLOG_OBJ_INCLUDES) \
		$(SOLIDSYSLOG_C99_INCLUDES) -c $< -o $@

clean:
	rm -rf $(SOLIDSYSLOG_BUILD_DIR)

print-config:
	@echo CC=$(CC)
	@echo AR=$(AR)
	@echo TOOL_FAMILY=$(TOOL_FAMILY)
	@echo SOLIDSYSLOG_TARGET_CFLAGS=$(SOLIDSYSLOG_TARGET_CFLAGS)
	@echo SOLIDSYSLOG_DIALECT_CFLAGS=$(SOLIDSYSLOG_DIALECT_CFLAGS)
	@echo SOLIDSYSLOG_C99_INCLUDES=$(SOLIDSYSLOG_C99_INCLUDES)
	@echo SOLIDSYSLOG_PLATFORMS=$(SOLIDSYSLOG_PLATFORMS)
	@echo SOLIDSYSLOG_LIB=$(SOLIDSYSLOG_LIB)

-include $(SOLIDSYSLOG_OBJS:.o=.d)
