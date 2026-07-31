# Consumer smoke test, Make flavour: the Path B counterpart to CMakeLists.txt in
# this directory. Consumes solidsyslog.mk the way an integrator does and compiles
# what it names against the config headers in CONFIG_DIR. Cross builds have no
# linker script here, so compiling and archiving under someone else's toolchain
# is the check.
#
#   make -f ci/consumer-smoke/Makefile LWIP_PATH=/opt/lwip \
#        FREERTOS_KERNEL_PATH=/opt/freertos/kernel

HERE            := $(dir $(lastword $(MAKEFILE_LIST)))
SOLIDSYSLOG_DIR ?= $(abspath $(HERE)/../..)
PLATFORMS       ?= LwipRaw FreeRtos
CONFIG_DIR      ?= config
BUILD           ?= $(SOLIDSYSLOG_DIR)/build/consumer-smoke-make

CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar

ifndef LWIP_PATH
$(error LWIP_PATH is not set — the consumer supplies the upstream trees)
endif
ifndef FREERTOS_KERNEL_PATH
$(error FREERTOS_KERNEL_PATH is not set — the consumer supplies the upstream trees)
endif

SOLIDSYSLOG_PLATFORMS := $(PLATFORMS)
include $(SOLIDSYSLOG_DIR)/solidsyslog.mk

UPSTREAM_INCLUDES := \
	-I$(HERE)$(CONFIG_DIR) \
	-I$(LWIP_PATH)/src/include \
	-I$(FREERTOS_KERNEL_PATH)/include \
	-I$(FREERTOS_KERNEL_PATH)/portable/GCC/ARM_CM3

BASE_CFLAGS := -mcpu=cortex-m3 -mthumb -Wall -Wextra -Wpedantic -Werror

CORE_OBJS     := $(patsubst $(SOLIDSYSLOG_DIR)/%.c,$(BUILD)/%.o,$(SOLIDSYSLOG_CORE_SRCS))
PLATFORM_OBJS := $(patsubst $(SOLIDSYSLOG_DIR)/%.c,$(BUILD)/%.o,$(SOLIDSYSLOG_PLATFORM_SRCS))
OBJS          := $(CORE_OBJS) $(PLATFORM_OBJS) $(BUILD)/main.o

# The split the fragment exists to express: Core sees the library's own headers
# alone, the platform sources and consumer code also see the config headers.
$(CORE_OBJS): CFLAGS := $(BASE_CFLAGS) $(SOLIDSYSLOG_CORE_INCLUDES)
$(PLATFORM_OBJS) $(BUILD)/main.o: CFLAGS := $(BASE_CFLAGS) $(SOLIDSYSLOG_INCLUDES) $(UPSTREAM_INCLUDES)

.PHONY: all clean
all: $(BUILD)/libconsumer_smoke.a

$(BUILD)/libconsumer_smoke.a: $(OBJS)
	$(AR) rcs $@ $^

$(BUILD)/%.o: $(SOLIDSYSLOG_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/main.o: $(HERE)main.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)
