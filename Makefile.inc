ifndef DPP_ROOT
$(error DPP_ROOT is undefined)
endif

Q = @
ifeq ($(Q),)
CMAKE_Q = VERBOSE=1
endif
DPP_ROOT ?= .
LOCAL_MINGW64_BUILD_SCRIPT := $(DPP_ROOT)/mingw-w64-build/mingw-w64-build
LOCAL_MINGW64_BUILD_DIR := $(DPP_ROOT)/x86_64-w64-mingw32
LOCAL_MINGW64_CC := $(LOCAL_MINGW64_BUILD_DIR)/bin/x86_64-w64-mingw32-gcc
SIGNTOOL := osslsigncode
SIGNTOOL_PREFIX := codesign

INSTALL = install
CMAKE = cmake
CC = $(LOCAL_MINGW64_CC)
CXX = $(dir $(CC))x86_64-w64-mingw32-g++
DDK_INCLUDE_DIR = $(dir $(CC))../x86_64-w64-mingw32/include/ddk
CFLAGS := -Wall -Wextra -m64 -fPIC \
	-ffunction-sections -fdata-sections \
	-I$(DPP_ROOT) -I$(DDK_INCLUDE_DIR) \
	-D__INTRINSIC_DEFINED_InterlockedBitTestAndSet \
	-D__INTRINSIC_DEFINED_InterlockedBitTestAndReset
CXXFLAGS := -fno-exceptions -fno-rtti
EASTL_CXXFLAGS := -I$(DPP_ROOT)/EASTL/include -I$(DPP_ROOT)/EASTL/test/packages/EABase/include/Common \
	-DEASTL_THREAD_SUPPORT_AVAILABLE=0 \
	-DEASTL_EXCEPTIONS_ENABLED=0 \
	-DEASTL_ASSERT_ENABLED=0 \
	-DEA_COMPILER_NO_EXCEPTIONS=1 \
	-DEA_COMPILER_MANAGED_CPP=1 \
	-Wno-unknown-pragmas \
	-Wno-deprecated-copy
DRIVER_LDFLAGS := -shared \
	-Wl,--subsystem,native \
	-Wl,--image-base,0x140000000 \
	-Wl,--dynamicbase -Wl,--nxcompat \
	-Wl,--file-alignment,0x200 \
	-Wl,--section-alignment,0x1000 \
	-Wl,--stack,0x100000 \
	-Wl,--entry,DriverEntry \
	-Wl,--gc-sections \
	-nostartfiles -nostdlib
DRIVER_LIBS := -lntoskrnl -lhal
USER_LDFLAGS := -Wl,--dynamicbase -Wl,--nxcompat \
	-Wl,--gc-sections

DRIVER_ADDITIONAL_DEPS := $(DPP_ROOT)/DriverThread.cpp $(DPP_ROOT)/DriverThread.hpp
DRIVER_ADDITIONAL_OBJS := $(DPP_ROOT)/DriverThread.opp

EASTL_DEPS := $(wildcard $(DPP_ROOT)/EASTL/source/*.cpp) $(wildcard $(DPP_ROOT)/EASTL/include/EASTL/*.h)
EASTL_STATIC_LIB := $(DPP_ROOT)/EASTL-build/libEASTL.a

DRIVER_EASTL_COMPAT_DEPS := $(DPP_ROOT)/EASTL-compat/kcrt.cpp
DRIVER_EASTL_COMPAT := $(DPP_ROOT)/EASTL-compat/kcrt.opp

USER_EASTL_COMPAT_DEPS := $(DPP_ROOT)/EASTL-compat/ucrt.cpp
USER_EASTL_COMPAT := $(DPP_ROOT)/EASTL-compat/ucrt.opp

is_set = \
	$(if $1,, \
	$(error ERROR: $(if $2,$2)))

path_exists = \
	$(if $(realpath $1),, \
	$(error ERROR: $1 does not exist, run `make -C $(DPP_ROOT) deps` first.))

define CHECK_REQUIRED_PATHS
	$(call path_exists,$(CC))
	$(call path_exists,$(CXX))
	$(call path_exists,$(DDK_INCLUDE_DIR))
	$(call path_exists,$(DRIVER_ADDITIONAL_OBJS))
	$(call path_exists,$(EASTL_STATIC_LIB))
	$(call path_exists,$(DRIVER_EASTL_COMPAT))
	$(call path_exists,$(USER_EASTL_COMPAT))
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
	$(call CHECK_REQUIRED_PATHS)
	$(call is_set,$(1),First argument: Object files missing)
	$(call is_set,$(2),Second argument: Output object file missing)
	$(Q)$(CC) \
		$(CFLAGS) \
		$(DRIVER_LDFLAGS) \
		-o '$(2)' \
		$(1) \
		$(DRIVER_LIBS)
	@echo 'LD  $(2)'
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
	$(call CHECK_REQUIRED_PATHS)
	$(call is_set,$(1),First argument: Object files missing)
	$(call is_set,$(2),Second argument: Output object file missing)
	$(Q)$(CXX) \
		$(CFLAGS) \
		$(CXXFLAGS) \
		$(EASTL_CXXFLAGS) \
		$(DRIVER_LDFLAGS) \
		-o '$(2)' \
		$(1) \
		$(DRIVER_ADDITIONAL_OBJS) \
		$(DRIVER_EASTL_COMPAT) \
		$(EASTL_STATIC_LIB) \
		$(DRIVER_LIBS)
	@echo 'LD  $(2)'
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
		$(USER_EASTL_COMPAT) \
		$(EASTL_STATIC_LIB)
	@echo 'LD  $(2)'
endef

define INSTALL_EXEC
	$(call is_set,$(1),First argument: Executables to install missing)
	$(call is_set,$(DESTDIR),DESTDIR missing)
	$(INSTALL) -d '$(DESTDIR)/'
	for target in $(1); do \
		$(INSTALL) -s --strip-program='$(dir $(CC))/x86_64-w64-mingw32-strip' "$$target" '$(DESTDIR)'; \
	done
endef

define INSTALL_EXEC_SIGN
	$(call is_set,$(1),First argument: Executables to install missing)
	$(call is_set,$(DESTDIR),DESTDIR missing)
	$(MAKE) -C '$(DPP_ROOT)' -f Makefile.deps $(SIGNTOOL_PREFIX)
	$(INSTALL) -d '$(DESTDIR)/'
	test -x '$(shell which $(SIGNTOOL))' || { \
		printf '\n  *** %s ***\n\n' "$(SIGNTOOL) does not exist / not in your PATH / not executable."; \
		false; \
	}
	for target in $(1); do \
		rm -f "$(DESTDIR)/$$target"; \
		$(dir $(CC))/x86_64-w64-mingw32-strip -s "$$target"; \
		$(SIGNTOOL) sign -pkcs12 '$(DPP_ROOT)/$(SIGNTOOL_PREFIX)-code.p12' \
			-ac '$(DPP_ROOT)/$(SIGNTOOL_PREFIX)-ca-cert.pem' \
			-in "$$target" \
			-out "$(DESTDIR)/$$target"; \
	done
	$(INSTALL) "$(DPP_ROOT)/$(SIGNTOOL_PREFIX)-ca-cert.pem" '$(DESTDIR)/$(SIGNTOOL_PREFIX)-ca-cert.crt'
endef

define CHECK_DPP
	$(MAKE) -C $(DPP_ROOT) -f Makefile.deps all
endef
