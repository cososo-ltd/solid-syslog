# Consumer smoke test, Make flavour: the Path B counterpart to CMakeLists.txt in
# this directory. Selects sources by directory, the way a hand-written Makefile
# does, and compiles them against the integrator config headers in CONFIG_DIR.
# Cross builds have no linker script here, so compiling and archiving under
# someone else's toolchain is the check.
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

CORE_SRCS     := $(wildcard $(SOLIDSYSLOG_DIR)/Core/Source/*.c)
PLATFORM_SRCS := $(foreach p,$(PLATFORMS),$(wildcard $(SOLIDSYSLOG_DIR)/Platform/$(p)/Source/*.c))

SOLIDSYSLOG_INCLUDES := \
	-I$(SOLIDSYSLOG_DIR)/Core/Interface \
	-I$(SOLIDSYSLOG_DIR)/Core/Source \
	$(foreach p,$(PLATFORMS), \
		-I$(SOLIDSYSLOG_DIR)/Platform/$(p)/Interface \
		-I$(SOLIDSYSLOG_DIR)/Platform/$(p)/Source)

UPSTREAM_INCLUDES := \
	-I$(HERE)$(CONFIG_DIR) \
	-I$(LWIP_PATH)/src/include \
	-I$(FREERTOS_KERNEL_PATH)/include \
	-I$(FREERTOS_KERNEL_PATH)/portable/GCC/ARM_CM3

CFLAGS := -mcpu=cortex-m3 -mthumb -Wall -Wextra -Wpedantic -Werror \
	$(SOLIDSYSLOG_INCLUDES) $(UPSTREAM_INCLUDES)

OBJS := $(patsubst $(SOLIDSYSLOG_DIR)/%.c,$(BUILD)/%.o,$(CORE_SRCS) $(PLATFORM_SRCS)) \
	$(BUILD)/main.o

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
