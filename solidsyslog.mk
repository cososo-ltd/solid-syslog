# SolidSyslog integration fragment for Make consumers.
#
#   SOLIDSYSLOG_PLATFORMS := LwipRaw FreeRtos
#   include third_party/solid-syslog/solidsyslog.mk
#
# Sets the variables below and defines no rules, so the consumer keeps their own
# object layout, flag sets and archive step. Name no platform and you get Core
# alone.
#
# Core compiles against the library's own headers; the platform sources must
# compile with the consumer's flags and config headers (lwipopts.h,
# FreeRTOSConfig.h, mbedtls_config.h, ffconf.h), which is why the two are listed
# separately. Each adapter gates itself on the upstream option it needs, so a
# source the consumer's configuration does not enable compiles to nothing.
#
#   SOLIDSYSLOG_CORE_SRCS       Core sources
#   SOLIDSYSLOG_PLATFORM_SRCS   sources of the selected platforms
#   SOLIDSYSLOG_SRCS            both
#   SOLIDSYSLOG_CORE_INCLUDES   include set for compiling Core
#   SOLIDSYSLOG_INCLUDES        include set for the platform sources, and for
#                               consumer code calling the library

SOLIDSYSLOG_MK_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
SOLIDSYSLOG_DIR    ?= $(SOLIDSYSLOG_MK_DIR)

# A platform that does not exist would match no files and build cleanly, minus
# every adapter it was supposed to bring.
SOLIDSYSLOG_UNKNOWN := $(strip $(foreach p,$(SOLIDSYSLOG_PLATFORMS), \
	$(if $(wildcard $(SOLIDSYSLOG_DIR)/Platform/$(p)/Source),,$(p))))
ifneq ($(SOLIDSYSLOG_UNKNOWN),)
$(error SOLIDSYSLOG_PLATFORMS names an unknown platform: $(SOLIDSYSLOG_UNKNOWN). Available: \
$(notdir $(patsubst %/,%,$(dir $(wildcard $(SOLIDSYSLOG_DIR)/Platform/*/Source)))))
endif

SOLIDSYSLOG_CORE_SRCS     := $(wildcard $(SOLIDSYSLOG_DIR)/Core/Source/*.c)
SOLIDSYSLOG_PLATFORM_SRCS := $(foreach p,$(SOLIDSYSLOG_PLATFORMS), \
	$(wildcard $(SOLIDSYSLOG_DIR)/Platform/$(p)/Source/*.c))
SOLIDSYSLOG_SRCS          := $(SOLIDSYSLOG_CORE_SRCS) $(SOLIDSYSLOG_PLATFORM_SRCS)

SOLIDSYSLOG_CORE_INCLUDES := \
	-I$(SOLIDSYSLOG_DIR)/Core/Interface \
	-I$(SOLIDSYSLOG_DIR)/Core/Source

SOLIDSYSLOG_INCLUDES := $(SOLIDSYSLOG_CORE_INCLUDES) \
	$(foreach p,$(SOLIDSYSLOG_PLATFORMS), \
		-I$(SOLIDSYSLOG_DIR)/Platform/$(p)/Interface \
		-I$(SOLIDSYSLOG_DIR)/Platform/$(p)/Source)
