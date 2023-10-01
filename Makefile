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
	$(MAKE) -C examples DPP_ROOT=$(realpath $(DPP_ROOT)) BUILD_NATIVE=1 all

examples-clean:
	$(MAKE) -C examples DPP_ROOT=$(realpath $(DPP_ROOT)) clean
	$(MAKE) -C examples DPP_ROOT=$(realpath $(DPP_ROOT)) BUILD_NATIVE=1 clean

examples-install:
	$(MAKE) -C examples DPP_ROOT=$(realpath $(DPP_ROOT)) install
	$(MAKE) -C examples DPP_ROOT=$(realpath $(DPP_ROOT)) BUILD_NATIVE=1 install

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

package:
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps package

help:
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps help

.NOTPARALLEL: examples-clean deps-clean deps-distclean
.PHONY: examples deps deps-distclean deps-clean help
.DEFAULT_GOAL := deps
