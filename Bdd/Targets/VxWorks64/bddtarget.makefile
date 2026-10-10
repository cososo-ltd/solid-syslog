# Glue that builds the BDD target's archive inside the project's build, beside
# solidsyslog.makefile and after it, which exports the target flags this reads.
# Build-VxWorks64Vip.ps1 passes both files to make; neither is copied into the
# project.
#
#   BDD_TARGET_BUILD_DIR  where libsolidsyslogbdd.a is built (required)

ifndef BDD_TARGET_BUILD_DIR
$(error BDD_TARGET_BUILD_DIR is not set - name a directory for the build output)
endif

.PHONY: solidsyslog_bdd_target

solidsyslog_bdd_target:
	$(MAKE) -f $(SOLIDSYSLOG_DIR)/Bdd/Targets/VxWorks64/bddtarget-vxworks64.mk \
		CC="$(CC)" AR="$(AR)" TOOL_FAMILY="$(TOOL_FAMILY)" \
		SOLIDSYSLOG_BUILD_DIR="$(SOLIDSYSLOG_BUILD_DIR)" \
		SOLIDSYSLOG_PLATFORMS="$(SOLIDSYSLOG_PLATFORMS)" \
		BDD_TARGET_BUILD_DIR="$(BDD_TARGET_BUILD_DIR)" \
		bdd_target
