#
# Targets for building dependencies e.g. mingw-gcc/g++, STL, etc.
#

DPP_ROOT = .

ifndef JOBS
JOBS := 4
endif

export Q
export DPP_ROOT
export JOBS
export USE_MINGW64_TARBALL
export LOCAL_MINGW64_CC
export LOCAL_MINGW64_CXX
export LOCAL_MINGW64_RC
export SIGNTOOL
export SIGNTOOL_PREFIX

examples:
	$(MAKE) -C examples DPP_ROOT=$(realpath $(DPP_ROOT)) all

examples-clean:
	$(MAKE) -C examples DPP_ROOT=$(realpath $(DPP_ROOT)) clean

examples-install:
	$(MAKE) -C examples DPP_ROOT=$(realpath $(DPP_ROOT)) install

deps:
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps \
		WERROR=1
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps \
		BUILD_NATIVE=1 WERROR=1

deps-distclean:
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps distclean
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps BUILD_NATIVE=1 distclean

deps-clean:
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps clean
	$(MAKE) -C $(DPP_ROOT) BUILD_NATIVE=1 -f Makefile.deps clean

help:
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps help

.PHONY: examples deps deps-distclean deps-clean help
.DEFAULT_GOAL := deps
