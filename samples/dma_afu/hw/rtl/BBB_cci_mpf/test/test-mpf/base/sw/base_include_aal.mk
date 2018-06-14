##
## Define base source files
##

BASE_FILE_PATH = ../../base/sw
BASE_FILE_SRC = cci_test_main.cpp aal_svc_wrapper.cpp
BASE_FILE_INC = $(BASE_FILE_PATH)/aal_svc_wrapper.h

VPATH = .:$(BASE_FILE_PATH)

CPPFLAGS ?=
CXX	 ?= g++
LDFLAGS	 ?=

ifeq (,$(CFLAGS))
CFLAGS = -g -O2
endif

ifneq (,$(ndebug))
else
CPPFLAGS += -DENABLE_DEBUG=1
endif
ifneq (,$(nassert))
else
CPPFLAGS += -DENABLE_ASSERT=1
endif

ifeq (,$(DESTDIR))
ifneq (,$(prefix))
CPPFLAGS += -I$(prefix)/include
LDFLAGS	 += -L$(prefix)/lib -Wl,-rpath-link -Wl,$(prefix)/lib -Wl,-rpath -Wl,$(prefix)/lib \
	    -L$(prefix)/lib64 -Wl,-rpath-link -Wl,$(prefix)/lib64 -Wl,-rpath -Wl,$(prefix)/lib64
endif
else
ifeq (,$(prefix))
prefix = /usr/local
endif
CPPFLAGS += -I$(DESTDIR)$(prefix)/include
LDFLAGS	 += -L$(DESTDIR)$(prefix)/lib -Wl,-rpath-link -Wl,$(prefix)/lib -Wl,-rpath -Wl,$(DESTDIR)$(prefix)/lib \
	    -L$(DESTDIR)$(prefix)/lib64 -Wl,-rpath-link -Wl,$(prefix)/lib64 -Wl,-rpath -Wl,$(DESTDIR)$(prefix)/lib64
endif

CPPFLAGS += -I../../base/sw
LDFLAGS += -lboost_program_options

FPGA_LIBS = -lOSAL -lAAS -laalrt
ASE_LIBS = $(FPGA_LIBS)
