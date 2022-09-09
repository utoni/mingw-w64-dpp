#
# Targets for building dependencies e.g. mingw-gcc/g++, STL, etc.
#

DPP_ROOT = .

ifndef JOBS
JOBS := 4
endif

examples:
	$(MAKE) -C examples DPP_ROOT=$(realpath $(DPP_ROOT)) all

examples-clean:
	$(MAKE) -C examples DPP_ROOT=$(realpath $(DPP_ROOT)) clean

examples-install:
	$(MAKE) -C examples DPP_ROOT=$(realpath $(DPP_ROOT)) install

deps:
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps WERROR=1 JOBS=$(JOBS) Q=$(Q)
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps BUILD_NATIVE=1 WERROR=1 JOBS=$(JOBS) Q=$(Q)

deps-distclean:
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps distclean

deps-clean:
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps clean

help:
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps help

.PHONY: examples deps deps-distclean deps-clean help
.DEFAULT_GOAL := deps
