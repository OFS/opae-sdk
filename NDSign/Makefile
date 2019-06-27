#//START_MODULE_HEADER/////////////////////////////////////////////////////////
#
# $Header: //acds/main/quartus/devices/firmware/tools/sign/Makefile#46 $
#
# Description: This contains the S10 Firmware Signing Infra
#                 The Bitstream Spec will explain a lot of this [Owner:Andrew Draper]
#                 http://rd/ice/product/Nadder/Documentation/FS/Nadder_Config_Data.docx
#
#
# Authors:     Jeff DaSilva
#
#              Copyright (c) Intel 2017
#              All rights reserved.
#
#
#//END_MODULE_HEADER///////////////////////////////////////////////////////////


###############
#
#  To use this makefile, simply use qshell with nios2test and css arc resources
#   or do:
#    % arc shell acdskit nios2test css devmain
#
#  And then build and test with:
#   % gmake test
#
###############


###############################################################################
# Initial Environment Setup and Sanity Checks
###############################################################################

SHELL := /bin/bash
.SUFFIXES:
.SECONDEXPANSION:
.DELETE_ON_ERROR:

SPACE := $(empty) $(empty)
COMMA := ,

include ../../nd/bcommon/env.mk
ifeq ($(ACDS_HOST_SYS),windows)
WORK_DIR := ./work_dir
ARC_TOOLS ?= s:/tools
else
WORK_DIR := $(get_work_dir)
ARC_TOOLS ?= /tools
endif


ifneq ($(DISABLE_MAKE_WRAP_CHECK),1)
ifneq ($(MAKE_WRAP_SMART),)
$(error ERROR: MAKE_WRAP_SMART is set to $(MAKE_WRAP_SMART) which implies you are using the make perl wrapper. Try 'gmake' instead of 'make')
endif
endif

# On linux, add a check to ensure ltsign is in your PAHT
ifeq ($(ACDS_HOST_SYS),linux)
ifeq ($(shell which ltsign 2>/dev/null),)
$(error ERROR: ltsign not in PATH. Run 'arc shell css' to resolve)
endif
endif

PYTHON.SRC := \
	bitstream/__init__.py \
	bitstream/bitstream.py \
	bitstream/cmf.py \
	bitstream/cmf_descriptor.py \
	bitstream/engineering_cert.py \
	bitstream/engineering_cert_header.py \
	bitstream/factory.py \
	bitstream/fragment.py \
	bitstream/fragment_collection.py \
	bitstream/key.py \
	bitstream/magic.py \
	bitstream/padding.py \
	bitstream/section.py \
	bitstream/signature.py \
	bitstream/signature_chain.py \
	bitstream/signature_chain_block0_entry.py \
	bitstream/signature_chain_entry.py \
	bitstream/signature_chain_public_key_entry.py \
	bitstream/signature_chain_root_entry.py \
	bitstream/signature_chain_multi_root_entry.py \
	bitstream/signature_chain_section.py \
	bitstream/signature_descriptor.py \
	bitstream/zip.py \
	flows/Command.py \
	flows/CreateEngCertCommand.py \
	flows/CreateRootKeyCommand.py \
	flows/ExtractModuleCommand.py \
	flows/ObjdumpCommand.py \
	flows/ServerModeCommand.py \
	flows/SignCmfCommand.py \
	flows/SignCmfApproveCommand.py \
	flows/SignCmfFinalizeCommand.py \
	flows/SignCmfRequestCommand.py \
	flows/SignCommand.py \
	flows/SignEngCertCommand.py \
	flows/SignEngCertRequestCommand.py \
	flows/SignEngCertApproveCommand.py \
	flows/SignEngCertFinalizeCommand.py \
	flows/SignKeyApproveCommand.py \
	flows/SignKeyCommand.py \
	flows/SignEphCommand.py \
	flows/SignKeyFinalizeCommand.py \
	flows/SignKeyRequestCommand.py \
	flows/CreatePublicKeyModuleCommand.py \
	flows/SignModuleApproveCommand.py \
	flows/SignModuleFinalizeCommand.py \
	flows/SignModuleRequestCommand.py \
	flows/__init__.py \
	keys/__init__.py \
	keys/vault.py \
	ndsign.py \
	networking/__init__.py \
	networking/cmdrspserver.py \
	networking/cssserver.py \
	networking/server.py \
	sign/__init__.py \
	sign/css.py \
	sign/module.py \
	util/__init__.py \
	util/convert.py \
	util/crc.py \
	util/exttool.py \
	util/fileutil.py \
	util/mkdir.py

# Files not used by signing operations. Specifically they have no unittests and will cause
# the unittest step to fail.
PYTHON.UNRELATED_SRC := \
	cmf_sign/get-cmf_sign.py \
	cmf_sign/pefile-lib/ordlookup/__init__.py \
	cmf_sign/pefile-lib/ordlookup/oleaut32.py \
	cmf_sign/pefile-lib/ordlookup/ws2_32.py \
	cmf_sign/pefile-lib/pefile.py \
	cmf_sign/pefile-lib/peutils.py

PYTHON.DISCOVERED_PACKAGES = $(foreach subdir,* */* */*/* */*/*/*,$(patsubst %/__init__.py,%,$(wildcard $(subdir)/__init__.py)))
PYTHON.DISCOVERED_SRC := $(sort $(wildcard *.py) $(foreach pkg,$(PYTHON.DISCOVERED_PACKAGES),$(wildcard $(pkg)/*.py)))
PYTHON.UNUSED_SRC := $(filter-out $(PYTHON.SRC) $(PYTHON.UNRELATED_SRC),$(PYTHON.DISCOVERED_SRC))

ifneq ($(PYTHON.UNUSED_SRC),)
$(warning WARNING: These source files are not included in PYTHON.SRC: $(PYTHON.UNUSED_SRC))
endif
#############################

#############################
.PHONY: all
all: check

STAMP_DIR = $(WORK_DIR)/stamps

PYTHON.DISABLE_UNITTEST_SRC += %/__init__.py

ifeq ($(ACDS_HOST_SYS),linux)
# don't run these on linux because I don't want them to break the build just yet
PYTHON.DISABLE_UNITTEST_SRC += networking/%
endif

PYTHON.UNITTEST_SRC := $(filter-out $(PYTHON.DISABLE_UNITTEST_SRC),$(PYTHON.SRC))

PYTHON.UNITTEST_STAMPS := $(strip \
	$(patsubst %.py,$(STAMP_DIR)/%.unittest,\
	$(PYTHON.UNITTEST_SRC)\
	))

.PHONY: check
check: $(PYTHON.UNITTEST_STAMPS)

$(PYTHON.UNITTEST_STAMPS): $(STAMP_DIR)/%.unittest: %.py
	cd $(<D) && PYTHONPATH=$(abspath .) python -3 -B -m unittest $(*F)
	@mkdir -p $(@D)
	@touch $@
#############################


#############################
# add 'test' target for system level tests
include tests.mk
#############################

#############################
.PHONY: clean
clean:
	rm -f $(patsubst %.py,%.pyc,$(PYTHON.SRC))
	rm -rf *~ */*~ *.pyc */*.pyc *.cer
	rm -rf $(WORK_DIR) work_dir $(STAMP_DIR)
#############################


#############################
.PHONY: p4-edit-all
p4-edit-all:
	$(P4) edit ...

.PHONY: p4-revert-all
p4-revert-all:
	$(P4) revert -a ...

.PHONY: lint
lint: p4-edit-all remove-trailing-whitespace remove-windows-line-endings tabs2space p4-revert-all

.PHONY: tabs2space
tabs2space:
	@find . -type f -name '*.py' -exec sed -i 's/\t/    /g' {} \;

.PHONY: remove-trailing-whitespace
remove-trailing-whitespace:
	@find . -type f \( -name '*.py' -o -name 'Makefile' -o -name '*.mk' -o -name '*.pm' -o -name '*.txt' \) -exec sed -i 's/[ \t]*$$//g' {} \;

.PHONY: remove-windows-line-endings
remove-windows-line-endings:
ifeq ($(shell which dos2unix 2>/dev/null),)
	$(warning WARNING: dos2unix not installed)
else
	@find . -type f \( -name '*.py' -o -name 'Makefile' -o -name '*.mk' -o -name '*.pm' -o -name '*.txt' \) \
		-exec dos2unix {} \;
endif
#############################


#############################

NDSIGN_INSTALL_DIR = $(ARC_TOOLS)/ndsign/$(NDSIGN_VERSION)
NDSIGN_VERSION = $(subst $(SPACE),,$(shell grep 'Version =' ndsign.py | head -n1 | sed -e "s,.*Version[ \t]*\=[ \t]*'\([0-9a-zA-Z.]*\)',\1,"))
NDSIGN_UNDERSCORE_VERSION = _$(subst .,_,$(NDSIGN_VERSION))

ARC_RES_PUBLISH_DIR = $(shell $(P4) where //depot/devenv/resources | awk '{print $$3}')

.PHONY: arc-publish
arc-publish: lint test
ifeq ($(NDSIGN_VERSION),)
	$(error ERROR: NDSIGN_VERSION not set)
endif
ifeq ($(ARC_TOOLS),)
	$(error ERROR:  ARC_TOOLS not set)
endif
ifneq ($(FORCE),1)
	@if [ -d "$(NDSIGN_INSTALL_DIR)" ]; then echo ERROR: $(NDSIGN_INSTALL_DIR) already exists; false; fi
endif
	$(MAKE) clean
	@if [ -d $(NDSIGN_INSTALL_DIR) ]; then chmod -R 755 $(NDSIGN_INSTALL_DIR); fi
	@rm -rf $(NDSIGN_INSTALL_DIR)
	@mkdir -p $(NDSIGN_INSTALL_DIR)
	@cp -rfv *.bat *.py bitstream flows keys networking sign util $(NDSIGN_INSTALL_DIR)
	@cd $(NDSIGN_INSTALL_DIR) && ln -s ndsign.py ndsign
	@chmod -R 555 $(NDSIGN_INSTALL_DIR)

	@unlink $(ARC_TOOLS)/ndsign/latest
	@cd $(ARC_TOOLS)/ndsign && ln -s $(NDSIGN_VERSION) latest

	$(P4) sync //depot/devenv/resources/\* //depot/devenv/resources/ndsign/...
	cat arc_resource_publish.pm | \
		sed -e 's,__VERSION__,$(NDSIGN_VERSION),g' \
		    -e 's,__VERSION_UNDERSCORE__,$(NDSIGN_UNDERSCORE_VERSION),g' \
		> $(ARC_RES_PUBLISH_DIR)/ndsign/$(NDSIGN_UNDERSCORE_VERSION).pm
	cd $(ARC_RES_PUBLISH_DIR) && perl -c ndsign/$(NDSIGN_UNDERSCORE_VERSION).pm
	cd $(ARC_RES_PUBLISH_DIR) && ./define_resource ndsign/$(NDSIGN_VERSION)
	$(P4) add $(ARC_RES_PUBLISH_DIR)/ndsign/$(NDSIGN_UNDERSCORE_VERSION).pm
	$(P4) submit -d "publish ndsign version $(NDSIGN_VERSION)" $(ARC_RES_PUBLISH_DIR)/ndsign/$(NDSIGN_UNDERSCORE_VERSION).pm

#############################
