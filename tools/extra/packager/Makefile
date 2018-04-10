.PHONY: clean all
all: packager_zip wrapper
clean: clean-packager clean-wrapper

#
# Packager scripts
#
PACKAGER_DEST_DIR := $(ADAPT_DEST_ROOT)/tools/packager
PACKAGER_WORK_DIR := $(ADAPT_WORK_ROOT)/tools/packager

PACKAGER_FILES := afu.py \
		  gbs.py\
		  __main__.py \
		  utils.py \
		  metadata/metadata.py \
		  metadata/constants.py \
		  metadata/__init__.py \
		  schema/afu_schema_v01.json \
		  packager.py \
		  README

PACKAGER_FILES_WORK_DIR := $(addprefix $(PACKAGER_WORK_DIR)/,$(PACKAGER_FILES))

PACKAGER_TARGET_ZIP := $(PACKAGER_DEST_DIR)/packager.pyz
PACKAGER_WORK_ZIP := $(PACKAGER_WORK_DIR)/packager.zip
PACKAGER_LIB := $(PACKAGER_DEST_DIR)/lib

.PHONY: packager_zip
packager_zip: $(PACKAGER_TARGET_ZIP)

$(PACKAGER_TARGET_ZIP): $(PACKAGER_WORK_ZIP)
	mkdir -p $(PACKAGER_DEST_DIR)
	rm -f $(PACKAGER_TARGET_ZIP)
	echo '#!/usr/bin/env python' | cat - $(PACKAGER_WORK_ZIP) > $(PACKAGER_TARGET_ZIP)
	chmod +x $(PACKAGER_TARGET_ZIP)

$(PACKAGER_WORK_ZIP): $(PACKAGER_FILES_WORK_DIR)
	mkdir -p $(PACKAGER_WORK_DIR)
	rm -f $(PACKAGER_WORK_DIR)/packager.zip
	cd $(PACKAGER_WORK_DIR) && zip -9r packager.zip $(PACKAGER_FILES)

$(PACKAGER_FILES_WORK_DIR): $(PACKAGER_WORK_DIR)/%: %
	mkdir -p $(dir $@)
	cp -f $< $@

.PHONY: packager_lib
packager_lib:
	mkdir -p $(PACKAGER_LIB)
	cp -rf lib $(PACKAGER_DEST_DIR)/
	find $(PACKAGER_DEST_DIR) -name "*.pyc" | xargs rm -f

.PHONY: clean-packager
clean-packager:
	rm -rf $(PACKAGER_DEST_DIR)
	rm -rf $(PACKAGER_WORK_DIR)

.PHONY: test test_src test_zip
test: test_src test_zip

PY_TESTS := $(wildcard test/*.py)
define run_test
	PYTHONPATH=$(2) python $(1)

endef

test_src:
	$(foreach py_test,$(PY_TESTS),$(call run_test,$(py_test),.:./lib))

test_zip: $(PACKAGER_TARGET_ZIP) packager_lib
	$(foreach py_test,$(PY_TESTS),$(call run_test,$(py_test),$(PACKAGER_TARGET_ZIP):$(PACKAGER_DEST_DIR)/lib))

#
# Wrapper script
#
.PHONY: wrapper clean-wrapper
wrapper: $(ADAPT_DEST_ROOT)/bin/packager
$(ADAPT_DEST_ROOT)/bin/packager: packager.sh
	mkdir -p $(dir $@)
	cp -f $< $@

clean-wrapper:
	rm -f $(ADAPT_DEST_ROOT)/bin/packager
