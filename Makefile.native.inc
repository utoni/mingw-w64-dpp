ifndef DPP_ROOT
$(error DPP_ROOT is undefined)
endif

ifndef BUILD_NATIVE
$(error BUILD_NATIVE is _NOT_ defined, include Makefile.inc)
endif

NAME_SUFFIX := -native

Q = @
ifeq ($(Q),)
CMAKE_Q = VERBOSE=1
endif
DPP_ROOT = $(realpath .)

INSTALL = install
CMAKE = cmake
CC = /usr/bin/cc
CXX = /usr/bin/c++
CFLAGS := -Wall -Wextra -Wno-sign-compare -Wno-strict-aliasing \
	-m64 -fPIC -fvisibility=hidden \
	-ffunction-sections -fdata-sections -fno-builtin -ffreestanding \
	-I$(DPP_ROOT)/CRT
ifneq ($(WERROR),)
CFLAGS += -Werror
endif
CXXFLAGS := -fno-exceptions -fno-rtti -fuse-cxa-atexit
EASTL_CXXFLAGS := -I$(DPP_ROOT)/EASTL/include -I$(DPP_ROOT)/EASTL/test/packages/EABase/include/Common \
	-DEASTL_THREAD_SUPPORT_AVAILABLE=0 \
	-DEASTL_EXCEPTIONS_ENABLED=0 \
	-DEASTL_ASSERT_ENABLED=0 \
	-DEA_COMPILER_NO_EXCEPTIONS=1 \
	-DEA_COMPILER_MANAGED_CPP=1 \
	-Wno-unknown-pragmas \
	-Wno-deprecated-copy
USER_LDFLAGS := -Wl,--gc-sections

EASTL_DEPS := $(wildcard $(DPP_ROOT)/EASTL/source/*.cpp) $(wildcard $(DPP_ROOT)/EASTL/include/EASTL/*.h)
EASTL_BUILDDIR := EASTL-native-build
EASTL_STATIC_LIB := $(DPP_ROOT)/$(EASTL_BUILDDIR)/libEASTL.a

USER_CRT_DEPS := $(DPP_ROOT)/CRT/ucrt.cpp
USER_CRT := $(DPP_ROOT)/CRT/ucrt$(NAME_SUFFIX).opp

is_set = \
	$(if $1,, \
	$(error ERROR: $(if $2,$2)))

path_exists = \
	$(if $(realpath $1),, \
	$(error ERROR: $1 does not exist, run `make -C $(DPP_ROOT) deps` first.))

define CHECK_REQUIRED_PATHS
	$(call path_exists,$(CC))
	$(call path_exists,$(CXX))
	$(call path_exists,$(EASTL_STATIC_LIB))
	$(call path_exists,$(USER_CRT))
endef

define BUILD_C_OBJECT
	$(call CHECK_REQUIRED_PATHS)
	$(call is_set,$(1),First argument: Source file missing)
	$(call is_set,$(2),Second argument: Output object file missing)
	$(Q)$(CC) -std=c99 $(CFLAGS) -c $(1) -o $(2)
	@echo 'CC  $(2)'
endef

define BUILD_CPP_OBJECT
	$(call CHECK_REQUIRED_PATHS)
	$(call is_set,$(1),First argument: Source file missing)
	$(call is_set,$(2),Second argument: Output object file missing)
	$(Q)$(CXX) $(CFLAGS) $(CXXFLAGS) $(EASTL_CXXFLAGS) -c $(1) -o $(2)
	@echo 'CXX $@'
endef

define LINK_C_KERNEL_TARGET
	$(error BUILD_NATIVE does not support kernel targets.)
endef

define LINK_C_USER_TARGET
	$(call CHECK_REQUIRED_PATHS)
	$(call is_set,$(1),First argument: Object files missing)
	$(call is_set,$(2),Second argument: Output object file missing)
	$(Q)$(CC) \
		$(CFLAGS) \
		-o '$(2)' \
		$(1) \
		$(EASTL_STATIC_LIB) \
	@echo 'LD  $(2)'
endef

define LINK_CPP_KERNEL_TARGET
	$(error BUILD_NATIVE does not support kernel targets.)
endef

define LINK_CPP_USER_TARGET
	$(call CHECK_REQUIRED_PATHS)
	$(call is_set,$(1),First argument: Object files missing)
	$(call is_set,$(2),Second argument: Output object file missing)
	$(Q)$(CXX) \
		$(CFLAGS) \
		$(CXXFLAGS) \
		$(EASTL_CXXFLAGS) \
		$(USER_LDFLAGS) \
		-o '$(2)' \
		$(1) \
		$(USER_CRT) \
		$(EASTL_STATIC_LIB)
	@echo 'LD  $(2)'
endef

define INSTALL_EXEC
	$(call is_set,$(1),First argument: Executables to install missing)
	$(call is_set,$(DESTDIR),DESTDIR missing)
	$(INSTALL) -d '$(DESTDIR)/'
	for target in $(1); do \
		$(INSTALL) -s "$$target" '$(DESTDIR)'; \
	done
endef

define INSTALL_EXEC_SIGN
	$(error BUILD_NATIVE does not support code signing.)
endef

define CHECK_DPP
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps all
endef
