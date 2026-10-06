# The VxWorks 6.4 BDD target's own sources, built into libsolidsyslogbdd.a.
#
# The target is C99, like the rest of the BDD code, so it cannot be a project
# source: the project's flags select C89. It is compiled exactly as the
# library's platform sources are, with the dialect and flags
# solidsyslog-vxworks64.mk chooses, which this file includes for them. Takes the
# same variables as that file, plus:
#
#   BDD_TARGET_BUILD_DIR  objects and libsolidsyslogbdd.a are written here; kept
#                         apart from SOLIDSYSLOG_BUILD_DIR so the library's
#                         pattern rule cannot claim these objects
#
# Target: bdd_target.

SOLIDSYSLOG_BDDTARGET_MK := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
SOLIDSYSLOG_DIR ?= $(dir $(SOLIDSYSLOG_BDDTARGET_MK))../../..

ifndef BDD_TARGET_BUILD_DIR
$(error BDD_TARGET_BUILD_DIR is not set - name a directory for the build output)
endif

include $(SOLIDSYSLOG_DIR)/Platform/VxWorks64/solidsyslog-vxworks64.mk

BDD_TARGET_SRCS := $(SOLIDSYSLOG_DIR)/Bdd/Targets/VxWorks64/BddTargetVxWorks64.c \
	$(SOLIDSYSLOG_DIR)/Bdd/Targets/VxWorks64/BddTargetVxWorks64Clock.c \
	$(SOLIDSYSLOG_DIR)/Bdd/Targets/VxWorks64/BddTargetVxWorks64Store.c \
	$(SOLIDSYSLOG_DIR)/Bdd/Targets/Common/BddTargetCustomSd.c \
	$(SOLIDSYSLOG_DIR)/Bdd/Targets/Common/BddTargetErrorText.c \
	$(SOLIDSYSLOG_DIR)/Bdd/Targets/Common/BddTargetInteractive.c \
	$(SOLIDSYSLOG_DIR)/Bdd/Targets/Common/BddTargetIps.c \
	$(SOLIDSYSLOG_DIR)/Bdd/Targets/Common/BddTargetLanguage.c \
	$(SOLIDSYSLOG_DIR)/Bdd/Targets/Common/BddTargetMessageSettings.c \
	$(SOLIDSYSLOG_DIR)/Bdd/Targets/Common/BddTargetStoreSettings.c \
	$(SOLIDSYSLOG_DIR)/Bdd/Targets/Common/BddTargetSwitchConfig.c
BDD_TARGET_INCLUDES := -I$(SOLIDSYSLOG_DIR)/Bdd/Targets/VxWorks64 -I$(SOLIDSYSLOG_DIR)/Bdd/Targets/Common
BDD_TARGET_LIB  := $(BDD_TARGET_BUILD_DIR)/libsolidsyslogbdd.a
BDD_TARGET_OBJS := $(addprefix $(BDD_TARGET_BUILD_DIR)/, $(notdir $(BDD_TARGET_SRCS:.c=.o)))

vpath %.c $(sort $(dir $(BDD_TARGET_SRCS)))

.PHONY: bdd_target

bdd_target: $(BDD_TARGET_LIB)

$(BDD_TARGET_LIB): $(BDD_TARGET_OBJS)
	$(AR) crs $@ $(BDD_TARGET_OBJS)

$(BDD_TARGET_BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(SOLIDSYSLOG_TARGET_CFLAGS) $(SOLIDSYSLOG_DIALECT_CFLAGS) \
		$(SOLIDSYSLOG_PLATFORM_EXTRA_CFLAGS) $(SOLIDSYSLOG_DEPEND_CFLAGS) $(SOLIDSYSLOG_INCLUDES) \
		$(BDD_TARGET_INCLUDES) $(SOLIDSYSLOG_C99_INCLUDES) -c $< -o $@

-include $(BDD_TARGET_OBJS:.o=.d)
