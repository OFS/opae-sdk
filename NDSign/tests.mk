#//START_MODULE_HEADER/////////////////////////////////////////////////////////
#
# $Header: $
#
# Description: This makefile fragment stores the system_level tests for ndsign.
#              The main Makefile should do "include tests.mk"
#
# Authors:     Jeff DaSilva
#
#              Copyright (c) Intel 2017
#              All rights reserved.
#
#
#//END_MODULE_HEADER///////////////////////////////////////////////////////////


#############################
ifeq ($(WORK_DIR),)
$(error ERROR: WORK_DIR not set)
endif

ifeq ($(PSG_SWIP),)
$(error ERROR: PSG_SWIP not set. If you are on a Windows machine and don't have a qshell, you can specify this variable in .bashrc with network drive path)
endif

TEST_DIR = $(WORK_DIR)/ndsign_systemtest_$@
TEST_SHARE_DIR = $(WORK_DIR)/ndsign_systemtest_share

BOOTROM_CHECKER.ORIG = /tools/soc/clarke/bootrom/bin/REVB/checkwrap
BOOTROM_CHECKER = test/checkers/checkwrap
NADDERDUMP = _JAVA_OPTIONS= nadderdump
NDSIGN = python -B ndsign.py
NDSIGN_INTERACTIVE = winpty $(NDSIGN)
#############################


#############################
define test_setup
@rm -rf $(TEST_DIR)
@mkdir -p $(TEST_DIR)
endef

define test_teardown
endef
#############################


#############################
CSS_PROJECT = PSGTest1
CSS_ROOT_KEYFILE = keys/css/$(CSS_PROJECT)_Release_pubkey.bin
CSS_CODESIGN_KEYFILE = keys/css/$(CSS_PROJECT)_Debug_pubkey.bin
#############################


#############################
CSS_CONFIG_FILE = $(subst \,/,$(APPDATA))/CSS_HOME/Intel/LT\ CSS/Bin/CSSConfig.txt

# according to Kevin Jacobs, this line is no longer needed for test mode to work: "-e 's,^ssl=.*,ssl=false,g'""

.PHONY: test-mode
test-mode:
ifneq ($(wildcard $(CSS_CONFIG_FILE)),)
	sed -i \
	   -e 's,^databasename=hsd_csc.*,databasename=hsd_csc_new_dev,g' \
	   $(CSS_CONFIG_FILE)
	cat $(CSS_CONFIG_FILE)
	CodeSign --viewrolesproject --project $(CSS_PROJECT)
else
	$(error ERROR: CSSConfig.txt not found here: $(CSS_CONFIG_FILE))
endif

.PHONY: production-mode
production-mode:
ifneq ($(wildcard $(CSS_CONFIG_FILE)),)
ifneq ($(shell grep hsd_csc_new_dev $(CSS_CONFIG_FILE) 2>/dev/null),)
	sed -i \
	   -e 's,^databasename=hsd_csc.*,databasename=hsd_csc,g' \
	   -e 's,^ssl=.*,ssl=true,g' \
	   $(CSS_CONFIG_FILE)
	cat $(CSS_CONFIG_FILE)
	CodeSign --viewrolesproject --project STRATIX10ROOTKEY
	CodeSign --viewrolesproject --project STRATIX10LimitedSigningKEY
endif
else
	$(error ERROR: CSSConfig.txt not found here: $(CSS_CONFIG_FILE))
endif



#############################
NADDER_ZIP.SRC = $(strip $(firstword \
   $(wildcard $(ACDS_DEST_ROOT)/quartus/common/devinfo/programmer/firmware/nadder.zip)\
   $(wildcard $(PSG_SWIP)/releases/acds/$(if $(ACDS_VERSION),$(ACDS_VERSION),18.1)/current.windows/windows64/quartus/common/devinfo/programmer/firmware/nadder.zip)\
))

define get_nadder_zip
if [ ! -f $1/nadder.zip ]; then \
   mkdir -p $1; \
   cp $(if $(NADDER_ZIP.SRC),$(NADDER_ZIP.SRC),$(error ERROR: nadder.zip could not be found)) $1/nadder.zip; \
fi
endef

NADDER_ZIP = $(TEST_SHARE_DIR)/nadder.zip
$(NADDER_ZIP):
	$(call get_nadder_zip,$(@D))

#############################


#############################
WHOAMI = $(patsubst amr\\%,%,$(shell whoami))
MY_QSHELL_NADDER_ZIP = s:/data/$(WHOAMI)/qshell2/quartuskit/acds/quartus/common/devinfo/programmer/firmware/nadder.zip

ifeq ($(WHOAMI),jdasilva)
ifeq ($(shell hostname),jdasilva-MOBL)
EXTRA_NDSIGN_SIGNCMF_ARGS += --css-keyhash="0VZyoNq5ChG5+f++hHPUxMk4YvA="
endif
endif

.PHONY: sign-my-qshell-overlay
sign-my-qshell-overlay: production-mode
	$(test_setup)
ifeq ($(wildcard $(MY_QSHELL_NADDER_ZIP)),)
	$(error ERROR: nadder.zip does not exist here - $(MY_QSHELL_NADDER_ZIP))
endif
	$(NDSIGN_INTERACTIVE) --debug sign_cmf $(EXTRA_NDSIGN_SIGNCMF_ARGS) STRATIX10LimitedSigningKEY.qky $(MY_QSHELL_NADDER_ZIP) $(TEST_DIR)/nadder_signed.zip
	$(test_teardown)
#############################


#############################
.PHONY: test-servermode
test-servermode:
	$(test_setup)
	$(NDSIGN) create_root_key --signtool=ltsign $(TEST_DIR)/root.qky
	$(NDSIGN) sign_key --signtool=ltsign $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1.qky
	$(NDSIGN_INTERACTIVE) servermode --signtool=ltsign
	$(test_teardown)

JDS_LAPTOP_SERVER := 1750@jdasilva-mobl.amr.corp.intel.com
#JDS_LAPTOP_SERVER := localhost

JDS_DESKTOP_SERVER := 1750@sj-jdasilva-620.altera.priv.altera.com

#SIGN_SERVERS += localhost
SIGN_SERVERS += $(JDS_LAPTOP_SERVER)
SIGN_SERVERS += $(JDS_DESKTOP_SERVER)
#SIGN_SERVERS += 1750@sj-interactive3

.PHONY: test-sign_cmf-with-server
test-sign_cmf-with-server:
	$(test_setup)
	$(NDSIGN) create_root_key --signtool=ltsign $(TEST_DIR)/root.qky
	$(NDSIGN) sign_key --signtool=ltsign $(TEST_DIR)/root.qky  $(TEST_DIR)/codesign1.qky
	$(NDSIGN) sign_cmf --server="$(subst $(SPACE),:,$(SIGN_SERVERS))" --signtool=ltsign $(TEST_DIR)/codesign1.qky test/files/example.zip  $(TEST_DIR)/example_signed.zip
	$(test_teardown)

test-real:
	$(test_setup)
	$(NDSIGN) sign_cmf --server="1750@fakeserver1:$(JDS_LAPTOP_SERVER)" STRATIX10LimitedSigningKEY.qky test/files/example.cmf  $(TEST_DIR)/example_signed.cmf
	$(test_teardown)
#############################


#############################
TESTCASES += test-help
.PHONY: test-help
test-help:
	$(test_setup)
	$(NDSIGN) --help > $(TEST_DIR)/help.txt
	$(NDSIGN) create_root_key --help > $(TEST_DIR)/create_root_key-help.txt
	$(NDSIGN) sign_key --help > $(TEST_DIR)/sign_key-help.txt
	$(NDSIGN) sign_cmf --help > $(TEST_DIR)/sign_cmf-help.txt
	$(NDSIGN) objdump --help > $(TEST_DIR)/objdump-help.txt
	$(NDSIGN) sign_key_request --help > $(TEST_DIR)/sign_key_request-help.txt
	$(NDSIGN) sign_key_approve --help > $(TEST_DIR)/sign_key_approve-help.txt
	$(NDSIGN) sign_key_finalize --help > $(TEST_DIR)/sign_key_finalize-help.txt
	$(NDSIGN) extractmodule --help > $(TEST_DIR)/extractmodule-help.txt
	$(test_teardown)
#############################

#############################
TESTCASES += test-basic-sign
.PHONY: test-basic-sign
test-basic-sign:
	$(test_setup)
	$(NDSIGN) create_root_key --signtool=ltsign $(TEST_DIR)/root.qky
	$(NDSIGN) sign_key --signtool=ltsign $(TEST_DIR)/root.qky  $(TEST_DIR)/codesign1.qky
	$(NDSIGN) sign_cmf --signtool=ltsign $(TEST_DIR)/codesign1.qky test/files/example.cmf  $(TEST_DIR)/example_signed.cmf

	$(NADDERDUMP) $(TEST_DIR)/root.qky | tee $(TEST_DIR)/root.qky.nadderdump
	$(NADDERDUMP) $(TEST_DIR)/codesign1.qky | tee $(TEST_DIR)/codesign1.qky.nadderdump
	$(NADDERDUMP) test/files/example.cmf | tee  $(TEST_DIR)/example.cmf.nadderdump
	$(NADDERDUMP) $(TEST_DIR)/example_signed.cmf | tee  $(TEST_DIR)/example_signed.cmf.nadderdump

	$(NDSIGN) objdump $(TEST_DIR)/root.qky > $(TEST_DIR)/root.qky.objdump
	$(NDSIGN) objdump $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.objdump
	$(NDSIGN) objdump test/files/example.cmf >  $(TEST_DIR)/example.cmf.objdump
	$(NDSIGN) objdump $(TEST_DIR)/example_signed.cmf >  $(TEST_DIR)/example_signed.cmf.objdump

	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.extmod

ifeq ($(ACDS_HOST_SYS),linux)
	$(BOOTROM_CHECKER) -v -v -v -i $(TEST_DIR)/example_signed.cmf
else
ifneq ($(wildcard s:/data/$(notdir $(shell whoami))),)
	@mkdir -p $(shell cygpath s:/data/$(notdir $(shell whoami))/checkwrap_stage)
	@cp -f $(TEST_DIR)/example_signed.cmf $(shell cygpath s:/data/$(notdir $(shell whoami))/checkwrap_stage)
endif
endif
	$(test_teardown)


TESTCASES += test-basic-fm-sign
.PHONY: test-basic-fm-sign
test-basic-fm-sign:
	$(test_setup)
	$(NDSIGN) create_root_key --signtool=ltsign --multi=1 $(TEST_DIR)/root.qky
	$(NDSIGN) sign_key --signtool=ltsign $(TEST_DIR)/root.qky  $(TEST_DIR)/codesign1.qky
	$(NDSIGN) sign_cmf --signtool=ltsign $(TEST_DIR)/codesign1.qky test/files/example_fm.cmf  $(TEST_DIR)/example_fm_signed.cmf

	$(NADDERDUMP) $(TEST_DIR)/root.qky | tee $(TEST_DIR)/root.qky.nadderdump
	$(NADDERDUMP) $(TEST_DIR)/codesign1.qky | tee $(TEST_DIR)/codesign1.qky.nadderdump
	$(NADDERDUMP) test/files/example_fm.cmf | tee  $(TEST_DIR)/example_fm.cmf.nadderdump
	$(NADDERDUMP) $(TEST_DIR)/example_fm_signed.cmf | tee  $(TEST_DIR)/example_fm_signed.cmf.nadderdump

	$(NDSIGN) objdump $(TEST_DIR)/root.qky > $(TEST_DIR)/root.qky.objdump
	$(NDSIGN) objdump $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.objdump
	$(NDSIGN) objdump test/files/example_fm.cmf >  $(TEST_DIR)/example_fm.cmf.objdump
	$(NDSIGN) objdump $(TEST_DIR)/example_fm_signed.cmf >  $(TEST_DIR)/example_fm_signed.cmf.objdump

	# Check CMF Descriptor format
	grep '0x0004' $(TEST_DIR)/example_fm_signed.cmf.objdump | grep -q '0x00000020'
	# In FM, there're 8 signature chains in signature descriptor
	grep 'Number of Signature Chains: 8' $(TEST_DIR)/example_fm_signed.cmf.objdump

	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.extmod
#############################


#############################
TESTCASES += test-sign-perm-cancel
.PHONY: test-sign-perm-cancel
test-sign-perm-cancel:
	$(test_setup)

	$(NDSIGN) create_root_key --signtool=ltsign $(TEST_DIR)/root.qky
	$(NDSIGN) sign_key --permissions=0xdeadbeef --cancel_id=0xabcd1234 --signtool=ltsign $(TEST_DIR)/root.qky  $(TEST_DIR)/codesign1a.qky
	$(NDSIGN) objdump $(TEST_DIR)/codesign1a.qky | grep 'Permissions:' | tail -n1 | grep '0xdeadbeef'
	$(NDSIGN) objdump $(TEST_DIR)/codesign1a.qky | grep 'Cancel ID:' | tail -n1 | grep '0xabcd1234'

	$(NDSIGN) sign_key_request --permissions=0xdeadbeef --cancel_id=0xabcd1234 --signtool=ltsign $(TEST_DIR)/root.qky  $(TEST_DIR)/codesign1b.qky
	$(NDSIGN) objdump $(TEST_DIR)/codesign1b.qky | grep 'Permissions:' | tail -n1 | grep '0xdeadbeef'
	$(NDSIGN) objdump $(TEST_DIR)/codesign1b.qky | grep 'Cancel ID:' | tail -n1 | grep '0xabcd1234'

	$(NDSIGN) sign_key_request --permissions=0xffffffff --cancel_id=0xffffffff --signtool=ltsign $(TEST_DIR)/root.qky  $(TEST_DIR)/codesign1c.qky
	$(NDSIGN) objdump $(TEST_DIR)/codesign1c.qky | grep 'Permissions:' | tail -n1 | grep '0xffffffff'
	$(NDSIGN) objdump $(TEST_DIR)/codesign1c.qky | grep 'Cancel ID:' | tail -n1 | grep '0xffffffff'

	$(NDSIGN) sign_key_request --permissions=0 --cancel_id=0 --signtool=ltsign $(TEST_DIR)/root.qky  $(TEST_DIR)/codesign1d.qky
	
	# awk '{$1=$1;print}' will trim leading/trailing whitespace (needed because this grep operation returned nothing in Windows/grep 3.1)
	$(NDSIGN) objdump $(TEST_DIR)/codesign1d.qky | grep 'Permissions:' | tail -n1 | awk '{$$1=$$1;print}' | grep '0x0$$'
	$(NDSIGN) objdump $(TEST_DIR)/codesign1d.qky | grep 'Cancel ID:' | tail -n1 | awk '{$$1=$$1;print}' | grep '0x0$$'

	$(test_teardown)

#############################


#############################
TESTCASES += test-resign-nadder-zip
.PHONY: test-resign-nadder-zip
test-resign-nadder-zip: $(NADDER_ZIP)
	$(test_setup)
	$(NDSIGN) create_root_key --signtool=ltsign $(TEST_DIR)/root.qky
	$(NDSIGN) sign_key --signtool=ltsign $(TEST_DIR)/root.qky  $(TEST_DIR)/codesign1.qky
	$(NDSIGN) sign_cmf --signtool=ltsign $(TEST_DIR)/codesign1.qky $(NADDER_ZIP) $(TEST_DIR)/nadder_signed.zip
	$(NDSIGN) objdump $(TEST_DIR)/nadder_signed.zip >  $(TEST_DIR)/nadder_signed.zip.objdump

	cd $(TEST_DIR) && unzip nadder_signed.zip nd5revb/SHA_384/main.cmf
	$(NDSIGN) objdump $(TEST_DIR)/nd5revb/SHA_384/main.cmf > $(TEST_DIR)/nadder_signed_zip_cmf.objdump

ifeq ($(ACDS_HOST_SYS),linux)
	$(BOOTROM_CHECKER) -v -v -v -i $(TEST_DIR)/nd5revb/SHA_384/main.cmf
endif

	$(test_teardown)
#############################

#############################
TESTCASES += test-http-objdump
.PHONY: test-http-objdump
test-http-objcopy:
	$(test_setup)
	$(NDSIGN) objdump https://sj-arc.altera.com/tools/acds/17.1/current.windows/windows64/quartus/common/devinfo/programmer/firmware/nadder.zip > $(TEST_DIR)/nadder_signed.zip.objdump
	$(test_teardown)
#############################

#############################
TESTCASES += test-extractmodule
.PHONY: test-extractmodule
test-extractmodule:
	$(test_setup)
	$(NDSIGN) create_root_key --signtool=ltsign $(TEST_DIR)/root.qky
	$(NDSIGN) sign_key --signtool=ltsign --permissions=0x1 --cancel_id=0x1 $(TEST_DIR)/root.qky $(TEST_DIR)/limitedcodesign1.qky
	$(NDSIGN) extractmodule $(TEST_DIR)/limitedcodesign1.qky > $(TEST_DIR)/limitedcodesign1.qky.1.extmod
	$(NDSIGN) extractmodule $(TEST_DIR)/limitedcodesign1.qky - > $(TEST_DIR)/limitedcodesign1.qky.2.extmod
	diff $(TEST_DIR)/limitedcodesign1.qky.1.extmod $(TEST_DIR)/limitedcodesign1.qky.2.extmod
	$(NDSIGN) extractmodule $(TEST_DIR)/limitedcodesign1.qky $(TEST_DIR)/limitedcodesign1.module
	$(NDSIGN) objdump $(TEST_DIR)/limitedcodesign1.module > $(TEST_DIR)/limitedcodesign1.module.objdump
	$(test_teardown)
#############################

#############################
TESTCASES += test-two-signature-sign
.PHONY: test-two-signature-sign
test-two-signature-sign:
	$(test_setup)
	$(NDSIGN) create_root_key --signtool=ltsign $(TEST_DIR)/root1.qky
	$(NDSIGN) sign_key --signtool=ltsign $(TEST_DIR)/root1.qky  $(TEST_DIR)/codesign1.qky
	$(NDSIGN) sign_key --signtool=ltsign $(TEST_DIR)/codesign1.qky $(TEST_DIR)/codesign2.qky
	$(NDSIGN) sign_cmf --signtool=ltsign $(TEST_DIR)/codesign2.qky test/files/example.cmf  $(TEST_DIR)/example_signed.cmf

	$(NDSIGN) create_root_key --signtool=ltsign $(TEST_DIR)/root2.qky
	$(NDSIGN) sign_key --signtool=ltsign $(TEST_DIR)/root2.qky $(TEST_DIR)/codesign1_secondary.qky
	$(NDSIGN) sign_key --signtool=ltsign $(TEST_DIR)/codesign1_secondary.qky $(TEST_DIR)/codesign2_secondary.qky
	$(NDSIGN) sign_cmf --signtool=ltsign $(TEST_DIR)/codesign2_secondary.qky $(TEST_DIR)/example_signed.cmf  $(TEST_DIR)/example_signed_x2.cmf

	$(NADDERDUMP) $(TEST_DIR)/example_signed_x2.cmf | tee  $(TEST_DIR)/example_signed_x2.cmf.nadderdump
	$(NDSIGN) objdump $(TEST_DIR)/example_signed_x2.cmf >  $(TEST_DIR)/example_signed_x2.cmf.objdump

ifeq ($(ACDS_HOST_SYS),linux)
	$(BOOTROM_CHECKER) -v -v -v -i $(TEST_DIR)/example_signed_x2.cmf
endif
	$(test_teardown)
#############################

#############################
TESTCASES += test-ltsign-req-approve-finalize
.PHONY: test-ltsign-req-approve-finalize
test-ltsign-req-approve-finalize:
	$(test_setup)
	$(NDSIGN) --verbose create_root_key --signtool=ltsign $(TEST_DIR)/root.qky
	$(NDSIGN) --verbose sign_key_request --permissions=0x1 --cancel_id=0x1 --signtool=ltsign $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN) --verbose sign_key_approve --signtool=ltsign --request_id=0 --approval_type=reviewer $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN) --verbose sign_key_approve --signtool=ltsign --request_id=0 --approval_type=manager $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN) --verbose sign_key_approve --signtool=ltsign --request_id=0 --approval_type=security $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN) --verbose sign_key_finalize --signtool=ltsign --request_id=0  --signkeyfile=$(TEST_DIR)/root.kp $(TEST_DIR)/codesign1_unsigned.qky $(TEST_DIR)/codesign1_signed.qky

	cp $(TEST_DIR)/codesign1_unsigned.qky $(TEST_DIR)/codesign1.qky
	$(NDSIGN) --verbose sign_key_finalize --signtool=ltsign --request_id=0  --signkeyfile=$(TEST_DIR)/root.kp $(TEST_DIR)/codesign1.qky

	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1_unsigned.qky > $(TEST_DIR)/codesign1_unsigned.qky.extmod
	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1_signed.qky > $(TEST_DIR)/codesign1_signed.qky.extmod
	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.extmod

	$(NDSIGN) objdump $(TEST_DIR)/codesign1_unsigned.qky > $(TEST_DIR)/codesign1_unsigned.qky.objdump
	$(NDSIGN) objdump $(TEST_DIR)/codesign1_signed.qky > $(TEST_DIR)/codesign1_signed.qky.objdump
	$(NDSIGN) objdump $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.objdump

	diff $(TEST_DIR)/codesign1.qky.extmod $(TEST_DIR)/codesign1_signed.qky.extmod
	diff $(TEST_DIR)/codesign1_unsigned.qky.extmod $(TEST_DIR)/codesign1_signed.qky.extmod

	$(test_teardown)
#############################

#############################
TESTCASES += test-ltsign-sign-module-flow
.PHONY: test-ltsign-sign-module-flow
test-ltsign-sign-module-flow:
	$(test_setup)
	$(NDSIGN) --verbose create_root_key --signtool=ltsign $(TEST_DIR)/root.qky
	$(NDSIGN) --verbose create_root_key --signtool=ltsign $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN) --verbose extractmodule $(TEST_DIR)/codesign1_unsigned.qky $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_request --permissions=0x1 --cancel_id=0x1 --signtool=ltsign $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=reviewer $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=manager $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=security $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_finalize --signtool=ltsign --request_id=0 $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.module $(TEST_DIR)/codesign1_signed.qky

	cp $(TEST_DIR)/codesign1_unsigned.qky $(TEST_DIR)/codesign1.qky
	$(NDSIGN) --verbose sign_module_finalize --signtool=ltsign --request_id=0 $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.module $(TEST_DIR)/codesign1.qky

	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1_unsigned.qky > $(TEST_DIR)/codesign1_unsigned.qky.extmod
	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1_signed.qky > $(TEST_DIR)/codesign1_signed.qky.extmod
	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.extmod

	$(NDSIGN) objdump $(TEST_DIR)/codesign1_unsigned.qky > $(TEST_DIR)/codesign1_unsigned.qky.objdump
	$(NDSIGN) objdump $(TEST_DIR)/codesign1_signed.qky > $(TEST_DIR)/codesign1_signed.qky.objdump
	$(NDSIGN) objdump $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.objdump

	diff $(TEST_DIR)/codesign1.qky.extmod $(TEST_DIR)/codesign1_signed.qky.extmod
	diff $(TEST_DIR)/codesign1_unsigned.qky.extmod $(TEST_DIR)/codesign1_signed.qky.extmod

	$(test_teardown)
#############################

#############################
TESTCASES += test-create-multi-root-key
.PHONY: test-create-multi-root-key
test-create-multi-root-key:
	$(test_setup)

	$(NDSIGN) --verbose create_root_key --signtool=ltsign --multi=1 --hash-sel=2 $(TEST_DIR)/root.qky

	$(NDSIGN) objdump --raw=1 $(TEST_DIR)/root.qky > $(TEST_DIR)/root.qky.objdump
	# Check multi-root entry magic
	grep '0x0000' $(TEST_DIR)/root.qky.objdump | grep -q '0x36733624'
	# Check hash select value
	grep '0x0014' $(TEST_DIR)/root.qky.objdump | grep -q '0x00000002'
	# Check public key magic
	grep '0x0020' $(TEST_DIR)/root.qky.objdump | grep -q '0x40656643'
	# Check fuse enablement and contribution fields.
	grep '0x0024' $(TEST_DIR)/root.qky.objdump | grep -q '0x00000000'
	grep '0x0028' $(TEST_DIR)/root.qky.objdump | grep -q '0xffffffff'

	$(test_teardown)
#############################

#############################
TESTCASES += test-ltsign-req-approve-finalize-multi-root-key
.PHONY: test-ltsign-req-approve-finalize-multi-root-key
test-ltsign-req-approve-finalize-multi-root-key:
	$(test_setup)

	$(NDSIGN) --verbose create_root_key --signtool=ltsign --multi=1 --hash-sel=0 $(TEST_DIR)/root.qky
	$(NDSIGN) --verbose sign_key_request --permissions=0x1 --cancel_id=0x1 --signtool=ltsign $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN) --verbose sign_key_approve --signtool=ltsign --request_id=0 --approval_type=reviewer $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN) --verbose sign_key_approve --signtool=ltsign --request_id=0 --approval_type=manager $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN) --verbose sign_key_approve --signtool=ltsign --request_id=0 --approval_type=security $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN) --verbose sign_key_finalize --signtool=ltsign --request_id=0  --signkeyfile=$(TEST_DIR)/root.kp $(TEST_DIR)/codesign1_unsigned.qky $(TEST_DIR)/codesign1_signed.qky

	cp $(TEST_DIR)/codesign1_unsigned.qky $(TEST_DIR)/codesign1.qky
	$(NDSIGN) --verbose sign_key_finalize --signtool=ltsign --request_id=0  --signkeyfile=$(TEST_DIR)/root.kp $(TEST_DIR)/codesign1.qky

	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1_unsigned.qky > $(TEST_DIR)/codesign1_unsigned.qky.extmod
	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1_signed.qky > $(TEST_DIR)/codesign1_signed.qky.extmod
	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.extmod

	$(NDSIGN) objdump $(TEST_DIR)/codesign1_unsigned.qky > $(TEST_DIR)/codesign1_unsigned.qky.objdump
	$(NDSIGN) objdump $(TEST_DIR)/codesign1_signed.qky > $(TEST_DIR)/codesign1_signed.qky.objdump
	$(NDSIGN) objdump $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.objdump

	diff $(TEST_DIR)/codesign1.qky.extmod $(TEST_DIR)/codesign1_signed.qky.extmod
	diff $(TEST_DIR)/codesign1_unsigned.qky.extmod $(TEST_DIR)/codesign1_signed.qky.extmod

	$(test_teardown)
#############################

#############################
TESTCASES += test-ltsign-sign-module-flow-with-multi-root-qky
.PHONY: test-ltsign-sign-module-flow-with-multi-root-qky
test-ltsign-sign-module-flow-with-multi-root-qky:
	$(test_setup)
	$(NDSIGN) --verbose create_root_key --signtool=ltsign --multi=1 --hash-sel=0 $(TEST_DIR)/root.qky
	$(NDSIGN) --verbose create_root_key --signtool=ltsign $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN) --verbose extractmodule $(TEST_DIR)/codesign1_unsigned.qky $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_request --permissions=0x1 --cancel_id=0x1 --signtool=ltsign $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=reviewer $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=manager $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=security $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_finalize --signtool=ltsign --request_id=0 $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.module $(TEST_DIR)/codesign1_signed.qky

	cp $(TEST_DIR)/codesign1_unsigned.qky $(TEST_DIR)/codesign1.qky
	$(NDSIGN) --verbose sign_module_finalize --signtool=ltsign --request_id=0 $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.module $(TEST_DIR)/codesign1.qky

	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1_unsigned.qky > $(TEST_DIR)/codesign1_unsigned.qky.extmod
	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1_signed.qky > $(TEST_DIR)/codesign1_signed.qky.extmod
	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.extmod

	$(NDSIGN) objdump $(TEST_DIR)/codesign1_unsigned.qky > $(TEST_DIR)/codesign1_unsigned.qky.objdump
	$(NDSIGN) objdump $(TEST_DIR)/codesign1_signed.qky > $(TEST_DIR)/codesign1_signed.qky.objdump
	$(NDSIGN) objdump $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.objdump

	diff $(TEST_DIR)/codesign1.qky.extmod $(TEST_DIR)/codesign1_signed.qky.extmod
	diff $(TEST_DIR)/codesign1_unsigned.qky.extmod $(TEST_DIR)/codesign1_signed.qky.extmod

	$(NDSIGN) objdump --raw=1 $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.objdump
	# Check multi-root entry magic
	grep '0x0000' $(TEST_DIR)/codesign1.qky.objdump | grep -q '0x36733624'
	# Check hash select value
	grep '0x0014' $(TEST_DIR)/codesign1.qky.objdump | grep -q '0x00000000'
	# Check public key magic
	grep '0x0020' $(TEST_DIR)/codesign1.qky.objdump | grep -q '0x40656643'
	# Check fuse enablement and contribution fields.
	grep '0x0024' $(TEST_DIR)/codesign1.qky.objdump | grep -q '0x00000000'
	grep '0x0028' $(TEST_DIR)/codesign1.qky.objdump | grep -q '0xffffffff'

	$(test_teardown)
#############################

#############################
TESTCASES += test-ltsign-sign-create-multi-root-keychain
.PHONY: test-ltsign-sign-create-multi-root-keychain
test-ltsign-sign-create-multi-root-keychain:
	$(test_setup)

	$(NDSIGN) --verbose create_root_key --signtool=ltsign --multi=1 --hash-sel=0 $(TEST_DIR)/root.qky
	$(NDSIGN) --verbose create_pubkey_module --signtool=ltsign --multi=1 $(TEST_DIR)/codesign1.kp $(TEST_DIR)/codesign1_unsigned.module

	$(NDSIGN) --verbose sign_module_request --permissions=0x1 --cancel_id=0x1 --multi=1 --signtool=ltsign $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=reviewer --multi=1 $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=manager --multi=1 $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=security --multi=1 $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_finalize --signtool=ltsign --request_id=0 --multi=1 $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.module $(TEST_DIR)/codesign1_signed.qky
	$(NDSIGN) --verbose sign_module_finalize --signtool=ltsign --request_id=0 --multi=1 $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.module $(TEST_DIR)/codesign1.qky

	$(NDSIGN) objdump --raw=1 $(TEST_DIR)/codesign1_unsigned.module > $(TEST_DIR)/codesign1_unsigned.qky.extmod
	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1_signed.qky > $(TEST_DIR)/codesign1_signed.qky.extmod
	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.extmod

	$(NDSIGN) objdump $(TEST_DIR)/codesign1_signed.qky > $(TEST_DIR)/codesign1_signed.qky.objdump
	diff $(TEST_DIR)/codesign1.qky.extmod $(TEST_DIR)/codesign1_signed.qky.extmod

	$(NDSIGN) objdump --raw=1 $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.objdump
	# Check multi-root entry magic
	grep '0x0000' $(TEST_DIR)/codesign1.qky.objdump | grep -q '0x36733624'
	# Check hash select value
	grep '0x0014' $(TEST_DIR)/codesign1.qky.objdump | grep -q '0x00000000'
	# Check public key magic
	grep '0x0020' $(TEST_DIR)/codesign1.qky.objdump | grep -q '0x40656643'
	# Check fuse enablement and contribution fields.
	grep '0x0024' $(TEST_DIR)/codesign1.qky.objdump | grep -q '0x00000000'
	grep '0x0028' $(TEST_DIR)/codesign1.qky.objdump | grep -q '0xffffffff'

	$(test_teardown)
#############################

#############################
TESTCASES += test-ltsign-dual-sign-fm-cmf-with-multi-root-keys
.PHONY: test-ltsign-dual-sign-fm-cmf-with-multi-root-keys
test-ltsign-dual-sign-fm-cmf-with-multi-root-keys:
	$(test_setup)

	# Create an Intel Root Key 
	$(NDSIGN) --verbose create_root_key --signtool=ltsign --multi=1 --hash-sel=0 $(TEST_DIR)/root.qky

	# Create codesign key 1, based on the Intel Root Key
	$(NDSIGN) --verbose create_pubkey_module --signtool=ltsign --multi=1 $(TEST_DIR)/codesign1.kp $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_request --permissions=0x1 --cancel_id=0x1 --multi=1 --signtool=ltsign $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=reviewer --multi=1 $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=manager --multi=1 $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=security --multi=1 $(TEST_DIR)/codesign1_unsigned.module
	$(NDSIGN) --verbose sign_module_finalize --signtool=ltsign --request_id=0 --multi=1 $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.module $(TEST_DIR)/codesign1.qky

	# Create a Manufacturing Root Key 
	$(NDSIGN) --verbose create_root_key --signtool=ltsign --multi=1 --hash-sel=3 $(TEST_DIR)/manuf_root.qky

	# Create codesign key 2, based on the Manufacturing Root Key
	$(NDSIGN) --verbose create_pubkey_module --signtool=ltsign --multi=1 $(TEST_DIR)/codesign1.kp $(TEST_DIR)/codesign2_unsigned.module
	$(NDSIGN) --verbose sign_module_request --permissions=0x1 --cancel_id=0x1 --multi=1 --signtool=ltsign $(TEST_DIR)/root.qky $(TEST_DIR)/codesign2_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=reviewer --multi=1 $(TEST_DIR)/codesign2_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=manager --multi=1 $(TEST_DIR)/codesign2_unsigned.module
	$(NDSIGN) --verbose sign_module_approve --signtool=ltsign --request_id=0 --approval_type=security --multi=1 $(TEST_DIR)/codesign2_unsigned.module
	$(NDSIGN) --verbose sign_module_finalize --signtool=ltsign --request_id=0 --multi=1 $(TEST_DIR)/root.qky $(TEST_DIR)/codesign2_unsigned.module $(TEST_DIR)/codesign2.qky

	# Signing
	$(NDSIGN) sign_cmf --signtool=ltsign $(TEST_DIR)/codesign1.qky test/files/example_fm.cmf  $(TEST_DIR)/example_fm_signed.cmf
	$(NDSIGN) sign_cmf --signtool=ltsign $(TEST_DIR)/codesign1.qky $(TEST_DIR)/example_fm_signed.cmf  $(TEST_DIR)/example_fm_dual_signed.cmf

	# Validation
	$(NDSIGN) objdump $(TEST_DIR)/example_fm_dual_signed.cmf >  $(TEST_DIR)/example_fm_dual_signed.cmf.objdump

	# Check CMF Descriptor format
	grep '0x0004:' $(TEST_DIR)/example_fm_dual_signed.cmf.objdump | grep -q '0x00000020'
	# In FM, there're 8 signature chains in signature descriptor
	grep 'Number of Signature Chains: 8' $(TEST_DIR)/example_fm_dual_signed.cmf.objdump

	# Check offsets in the Signature Chain Section Header
	grep '0x0000:' $(TEST_DIR)/example_fm_dual_signed.cmf.objdump | grep -q '0x00000060'
	grep '0x0004:' $(TEST_DIR)/example_fm_dual_signed.cmf.objdump | grep -q '0x00000280'

	# Look for magic number of multi root entry. There should be two occurrence. 
	grep '0x0000:' $(TEST_DIR)/example_fm_dual_signed.cmf.objdump | grep '0x36733624'

	$(test_teardown)
#############################

#############################
TESTCASES +=  test-rbf-sign
.PHONY: test-rbf-sign
test-rbf-sign:
	$(test_setup)
	$(NDSIGN) create_root_key --signtool=ltsign $(TEST_DIR)/root.qky
	$(NDSIGN) sign_key --signtool=ltsign $(TEST_DIR)/root.qky  $(TEST_DIR)/codesign1.qky
	$(NDSIGN) sign_cmf --signtool=ltsign $(TEST_DIR)/codesign1.qky test/files/example.rbf $(TEST_DIR)/example_signed_cmf.rbf
	$(NDSIGN) objdump test/files/example.rbf > $(TEST_DIR)/example_rbf.objdump
	$(NDSIGN) objdump $(TEST_DIR)/example_signed_cmf.rbf > $(TEST_DIR)/example_signed_cmf_rbf.objdump
	$(test_teardown)
#############################


#############################
TESTCASES +=  test-rbf-objdump
.PHONY: test-rbf-objdump
test-rbf-objdump:
	$(test_setup)
	$(NDSIGN) objdump test/files/example.rbf > $(TEST_DIR)/example_rbf.objdump
	cat $(TEST_DIR)/example_rbf.objdump | head -n 30
	$(test_teardown)
#############################

#############################
# Pull the version out of the cmf and verify all the pieces match what's expected
# There's an optional patch number on the end so test with and without.
TESTCASES += test-extract-version
.PHONY: test-extract-version
test-extract-version:
	$(test_setup)
	$(NDSIGN) objdump test/files/example_patch.cmf > $(TEST_DIR)/example_patch.cmf.objdump && \
	grep -q 'Version: 18.0 b148 p67' $(TEST_DIR)/example_patch.cmf.objdump
	$(NDSIGN) objdump test/files/example.cmf > $(TEST_DIR)/example.cmf.objdump && \
	grep -q 'Version: 17.1 b18' $(TEST_DIR)/example.cmf.objdump
	$(NDSIGN) objdump test/files/example_fm.cmf > $(TEST_DIR)/example_fm.cmf.objdump && \
	grep -q 'Version: 19.1 b88' $(TEST_DIR)/example_fm.cmf.objdump
	$(test_teardown)
#############################

#############################
# Test the engineering certificate create+sign flow
# ROOT_KEY is the fake altera/intel root key
# ENG_KEY is the engineering key that's signed by the root key
# CERT is the cert. It has the engineering key hash embedded in it, and is signed by the root key
# Later, engineering firmware would be signed using the engineering key and the engineering bootloader and cert would be prepended to the bitstream.
# This would allow the engineering firmware to run on an engineering device (has some fuses blown and matches the UID and HMAC info) without being signed by the root key.
# See Nadder_Config_Data.docx spec section 7.6.4.2
ENG_CERT_TEST.UID := 0x1102030405060708
ENG_CERT_TEST.HMAC := 0x1102030405060708091011121314151617181920212223242526272829303132
ENG_CERT_TEST.ROOT_KEY :=$(TEST_DIR)/eng_cert_test_root_key.qky
ENG_CERT_TEST.ENG_KEY :=$(TEST_DIR)/eng_cert_test_eng_key.qky
ENG_CERT_TEST.CERT := $(TEST_DIR)/eng_cert_test_eng_cert.eng

TESTCASES += test-create-eng-cert
.PHONY: test-create-eng-cert
test-create-eng-cert:
	$(test_setup)
	@# Setup
	$(NDSIGN) create_root_key --signtool=ltsign $(ENG_CERT_TEST.ROOT_KEY)
	$(NDSIGN) sign_key --signtool=ltsign $(ENG_CERT_TEST.ROOT_KEY) $(ENG_CERT_TEST.ENG_KEY)
	$(NDSIGN) --debug create_eng_cert $(ENG_CERT_TEST.ENG_KEY) $(ENG_CERT_TEST.UID) $(ENG_CERT_TEST.HMAC) $(ENG_CERT_TEST.CERT).unsigned
	$(NDSIGN) sign_eng_cert --signtool=ltsign $(ENG_CERT_TEST.ROOT_KEY) $(ENG_CERT_TEST.CERT).unsigned $(ENG_CERT_TEST.CERT)
	$(NDSIGN) objdump $(ENG_CERT_TEST.ENG_KEY) > $(ENG_CERT_TEST.ENG_KEY).objdump
	$(NDSIGN) objdump $(ENG_CERT_TEST.CERT) > $(ENG_CERT_TEST.CERT).objdump

	@# Test 1 - cert should be 8192 bytes exactly
	wc -c $(ENG_CERT_TEST.CERT) | grep -q '^8192\s'

	@# Test 2 - cert uid and hmac should match
	grep 'UID' $(ENG_CERT_TEST.CERT).objdump | sort -u | awk '{print $$2}' | grep -q '^$(ENG_CERT_TEST.UID)$$'
	grep 'HMAC' $(ENG_CERT_TEST.CERT).objdump | sort -u | awk '{print $$2}' | grep -q '^$(ENG_CERT_TEST.HMAC)$$'

	@# Test 3 - test key hash is correctly embedded (the objdump pipeline swaps endianness because objdump on key prints in big-endian)
	@grep 'Key Hash' $(ENG_CERT_TEST.CERT).objdump | grep 'Key Hash' | sort -u | sed 's/.*Key Hash.*:\s*//' | sed 's/0*\s*(Big-Endian.*//' > $(ENG_CERT_TEST.CERT).hash
	@$(NDSIGN) objdump $(ENG_CERT_TEST.ROOT_KEY) | grep 'KeyHash' | sed 's/.*:\s*//' > $(ENG_CERT_TEST.ROOT_KEY).hash
	diff $(ENG_CERT_TEST.CERT).hash $(ENG_CERT_TEST.ROOT_KEY).hash | wc -l | grep -q 0

	@# Test 4 - signature chain should have something in it
	grep 'Number of Entries in Signature Chain' $(ENG_CERT_TEST.CERT).objdump | grep -q ': [^0]]'
	$(test_teardown)
#############################


#############################
ifneq ($(ACDS_HOST_SYS),linux)
MANUAL_TESTCASES +=  test-css-req-approve-finalize
endif
.PHONY: test-css-req-approve-finalize
test-css-req-approve-finalize:
	$(test_setup)
	$(NDSIGN_INTERACTIVE) --verbose create_root_key --signtool=css --keyfile=$(CSS_ROOT_KEYFILE) $(TEST_DIR)/root.qky
	$(NDSIGN_INTERACTIVE) --verbose sign_key_request --signtool=css --css-project=$(CSS_PROJECT) --keyfile=$(CSS_CODESIGN_KEYFILE) $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1_unsigned.qky

	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1_unsigned.qky > $(TEST_DIR)/codesign1_unsigned.qky.extmod

	$(NDSIGN_INTERACTIVE) --verbose --debug sign_key_approve --signtool=css --approval_type=reviewer1 $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN_INTERACTIVE) --verbose --debug sign_key_approve --signtool=css --approval_type=reviewer2 $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN_INTERACTIVE) --verbose --debug sign_key_approve --signtool=css --approval_type=reviewer3 $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN_INTERACTIVE) --verbose sign_key_approve --signtool=css --approval_type=manager $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN_INTERACTIVE) --verbose sign_key_approve --signtool=css --approval_type=security $(TEST_DIR)/codesign1_unsigned.qky
	$(NDSIGN_INTERACTIVE) --verbose sign_key_finalize --signtool=css $(TEST_DIR)/codesign1_unsigned.qky $(TEST_DIR)/codesign1.qky

	$(NDSIGN) objdump $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.objdump
	$(NDSIGN) extractmodule $(TEST_DIR)/codesign1.qky > $(TEST_DIR)/codesign1.qky.extmod

	diff $(TEST_DIR)/codesign1_unsigned.qky.extmod $(TEST_DIR)/codesign1.qky.extmod

	$(NDSIGN_INTERACTIVE) --verbose sign_cmf --signtool=css --css-project=$(CSS_PROJECT) $(TEST_DIR)/codesign1.qky test/files/example.cmf $(TEST_DIR)/example_engineering_signed.cmf
	@mkdir -p $(shell cygpath s:/data/$(notdir $(shell whoami))/checkwrap_stage)
	@cp -f $(TEST_DIR)/example_engineering_signed.cmf $(shell cygpath s:/data/$(notdir $(shell whoami))/checkwrap_stage)

	$(test_teardown)
#############################


#############################
# Makes an eng cert that contains the public key for the "engineering key" (codesign2.qky") and is signed by the "project root key" (codesign1.qky)
ifneq ($(ACDS_HOST_SYS),linux)
MANUAL_TESTCASES +=  test-css-cert-approve-finalize
endif
.PHONY: test-css-cert-approve-finalize
test-css-cert-approve-finalize:
	$(test_setup)
	$(if $(REVIEWER2),,$(error Must set REVIEWER2 to run test-css-cert-approve-finalize))
	$(if $(REVIEWER3),,$(error Must set REVIEWER3 to run test-css-cert-approve-finalize))
	$(NDSIGN_INTERACTIVE) --verbose create_root_key --signtool=css --keyfile=$(CSS_ROOT_KEYFILE) $(TEST_DIR)/root.qky
	$(NDSIGN_INTERACTIVE) --debug --verbose sign_key --signtool=css --css-project=$(CSS_PROJECT) --keyfile=$(CSS_CODESIGN_KEYFILE) $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1.qky
	$(NDSIGN_INTERACTIVE) --debug --verbose sign_key --signtool=css --css-project=$(CSS_PROJECT) --keyfile=$(CSS_CODESIGN_KEYFILE) $(TEST_DIR)/codesign1.qky $(TEST_DIR)/codesign2.qky

	$(NDSIGN_INTERACTIVE) create_eng_cert $(TEST_DIR)/codesign2.qky  $(ENG_CERT_TEST.UID) $(ENG_CERT_TEST.HMAC)  $(TEST_DIR)/codesign2_eng_cert.cert
	$(NDSIGN_INTERACTIVE) --verbose sign_eng_cert_request --signtool=css --css-project=$(CSS_PROJECT) $(TEST_DIR)/codesign1.qky $(TEST_DIR)/codesign2_eng_cert.cert  $(TEST_DIR)/codesign1_eng_cert.cert
	$(NDSIGN) objdump $(TEST_DIR)/codesign1_eng_cert.cert > $(TEST_DIR)/codesign1_eng_cert.cert.objdump
	$(NDSIGN_INTERACTIVE) --verbose --debug sign_eng_cert_approve --signtool=css --approval_type=reviewer $(TEST_DIR)/codesign1_eng_cert.cert
	$(NDSIGN_INTERACTIVE) --verbose --debug sign_eng_cert_approve --signtool=css --approval_type=$(REVIEWER2) $(TEST_DIR)/codesign1_eng_cert.cert
	$(NDSIGN_INTERACTIVE) --verbose --debug sign_eng_cert_approve --signtool=css --approval_type=$(REVIEWER3) $(TEST_DIR)/codesign1_eng_cert.cert
	$(NDSIGN_INTERACTIVE) --verbose sign_eng_cert_approve --signtool=css --approval_type=manager $(TEST_DIR)/codesign1_eng_cert.cert
	$(NDSIGN_INTERACTIVE) --verbose sign_eng_cert_approve --signtool=css --approval_type=security $(TEST_DIR)/codesign1_eng_cert.cert
	$(NDSIGN_INTERACTIVE) --verbose sign_eng_cert_finalize --signtool=css $(TEST_DIR)/codesign1_eng_cert.cert $(TEST_DIR)/codesign1.cert

	$(NDSIGN) objdump $(TEST_DIR)/codesign1.cert > $(TEST_DIR)/codesign1.cert.objdump
	@echo Run this to see unsigned and signed certs compared: diff $(TEST_DIR)/codesign1_eng_cert.cert.objdump $(TEST_DIR)/codesign1.cert.objdump

	$(test_teardown)
#############################
# Test ephemeral signing
.PHONY: test-ephemeral-signing
TESTCASES += test-ephemeral-signing
EPHEMERAL_TEST.ROOT_KEY :=$(TEST_DIR)/ephemeral_test_root_key.qky

test-ephemeral-signing:
	$(test_setup)
	$(NDSIGN) create_root_key --signtool=ltsign $(EPHEMERAL_TEST.ROOT_KEY)
	$(NDSIGN) --verbose sign_ephemeral --signtool=ltsign $(EPHEMERAL_TEST.ROOT_KEY) test/files/example.zip $(TEST_DIR)/op.zip
	cd $(TEST_DIR) && unzip op.zip
	$(NDSIGN) objdump $(TEST_DIR)/foo/example.cmf > $(TEST_DIR)/example.cmf.objdump
	$(test_teardown)

# This test should only be run if you have a signing server resource as this requires a signing server to be defined in the environment variables
.PHONY: test-ephemeral-signing-server
test-ephemeral-signing-server:
	$(test_setup)
	$(NDSIGN) --verbose sign_ephemeral --signtool=css keys/css/STRATIX10LimitedSigningKEY.qky test/files/example.zip $(TEST_DIR)/op.zip
	$(test_teardown)

#############################
ifneq ($(ACDS_HOST_SYS),linux)
MANUAL_TESTCASES +=  test-css
endif
.PHONY: test-css
test-css:
	$(test_setup)
	$(NDSIGN_INTERACTIVE) create_root_key --signtool=css --keyfile=$(CSS_ROOT_KEYFILE) $(TEST_DIR)/root.qky
	$(NDSIGN_INTERACTIVE) --debug --verbose sign_key --signtool=css --css-project=$(CSS_PROJECT) --keyfile=$(CSS_CODESIGN_KEYFILE) $(TEST_DIR)/root.qky $(TEST_DIR)/codesign1.qky
	$(NDSIGN_INTERACTIVE) --debug --verbose sign_cmf --signtool=css --css-project=$(CSS_PROJECT) $(TEST_DIR)/codesign1.qky test/files/example.cmf $(TEST_DIR)/example_engineering_signed.cmf
	$(test_teardown)
#############################



#############################
.PHONY: test
test: check $(TESTCASES)
	@echo "All Tests Passed."

.PHONY: manual-tests
manual-tests: check $(MANUAL_TESTCASES)

.PHONY: test-all
test-all: test manual-tests

#############################


