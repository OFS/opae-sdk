
.SECONDEXPANSION:
.DELETE_ON_ERROR:

include ../../../nd/bcommon/env.mk
ND_ZIP_OFFSET := quartus/common/devinfo/programmer/firmware/nadder.zip

ifeq ($(wildcard $(ACDS_DEST_ROOT)/$(ND_ZIP_OFFSET)),)
ACDS_DEST_ROOT :=
endif

ifeq ($(ACDS_DEST_ROOT),)
ACDS_VERSION ?= 17.1
$(warning ACDS_DEST_ROOT not set to valid $(ACDS_VERSION) resource)
ACDS_HOST_SYS ?= windows
ifeq ($(ACDS_HOST_SYS),windows)
ACDS_DEST_ROOT := s:/tools/acds/$(ACDS_VERSION)/current.windows/windows64
else
ACDS_DEST_ROOT := $(if $(ARC_TOOLS),$(ARC_TOOLS),/tools)/acds/$(ACDS_VERSION)/current.$(ACDS_HOST_SYS)/$(ACDS_HOST_SYS)64
endif
$(info ACDS_DEST_ROOT set to $(ACDS_DEST_ROOT))
endif

ND_ZIP := $(ACDS_DEST_ROOT)/$(ND_ZIP_OFFSET)

CMF := files/example.cmf
#$(CMF).SRC = nd4/SHA_256/main.cmf
$(CMF).SRC = nd5revb/SHA_256/main.cmf


.PHONY: default
default: check

.PHONY: all
all: $(CMF)

.PHONY: check
check: check-$(CMF)

.PHONY: $(CMF)
$(CMF): $(ND_ZIP)
	@mkdir -p $(@D)
	$(P4) edit $@
	unzip -p $< $($(CMF).SRC) > $@
	$(P4) revert -a $@

.PHONY: check-$(CMF)
check-$(CMF): $(ND_ZIP) $(CMF)
	@rm -f $(CMF).chk
	unzip -p $< nadder_zip_sha256.chk | grep $($(CMF).SRC) | sed -e 's,$($(CMF).SRC),$(CMF),g' > $(CMF).chk
	sha256sum -c $(CMF).chk
	@rm $(CMF).chk
	@echo "make $@ [passed]"


.PHONY: clean
clean:
	rm -vf $(CMF).chk
