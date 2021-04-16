LOCAL_MINGW64_BUILD_SCRIPT := ./mingw-w64-build/mingw-w64-build
LOCAL_MINGW64_BUILD_DIR := ./x86_64-w64-mingw32
LOCAL_MINGW64_CC := $(LOCAL_MINGW64_BUILD_DIR)/bin/x86_64-w64-mingw32-gcc
LOCAL_MINGW64_DDK_INCLUDE_DIR := $(LOCAL_MINGW64_BUILD_DIR)/x86_64-w64-mingw32/include/ddk

CMAKE = cmake
CC = $(LOCAL_MINGW64_CC)
CXX = $(dir $(CC))/x86_64-w64-mingw32-g++
DDK_INCLUDE_DIR = $(LOCAL_MINGW64_DDK_INCLUDE_DIR)
CFLAGS := -Wall -Wextra -m64 -shared \
	-I$(DDK_INCLUDE_DIR) \
	-D__INTRINSIC_DEFINED_InterlockedBitTestAndSet \
	-D__INTRINSIC_DEFINED_InterlockedBitTestAndReset
CXXFLAGS := -fno-exceptions -fno-rtti
EASTL_CXXFLAGS := -IEASTL/include -IEASTL/test/packages/EABase/include/Common \
	-DEASTL_THREAD_SUPPORT_AVAILABLE=0 \
	-DEASTL_EXCEPTIONS_ENABLED=0 \
	-DEASTL_ASSERT_ENABLED=0 \
	-DEA_COMPILER_NO_EXCEPTIONS=1 \
	-DEA_COMPILER_MANAGED_CPP=1 \
	-Wno-unknown-pragmas \
	-Wno-deprecated-copy \
	-Wl,--gc-sections
EASTL_STATIC_LIB := EASTL-build/libEASTL.a
EASTL_COMPAT := EASTL-compat/kcrt.opp

1_DRIVER_NAME = ddk-template
1_OBJECTS = $(1_DRIVER_NAME).o
1_TARGET = $(1_DRIVER_NAME).sys

2_DRIVER_NAME = ddk-template-cplusplus
2_OBJECTS = $(2_DRIVER_NAME).opp
2_TARGET = $(2_DRIVER_NAME).sys

3_DRIVER_NAME = ddk-template-cplusplus-EASTL
3_OBJECTS = $(3_DRIVER_NAME).opp
3_TARGET = $(3_DRIVER_NAME).sys

all: deps-print-local-notice check-vars $(1_TARGET) $(2_TARGET) $(3_TARGET)

deps-print-local-notice: $(CC)
ifeq ($(CC),$(LOCAL_MINGW64_CC))
ifeq ($(DDK_INCLUDE_DIR),$(LOCAL_MINGW64_DDK_INCLUDE_DIR))
	@echo
	@echo "--------------------------------------------------------"
	@echo "-- You did not set CC and DDK_INCLUDE_DIR explicitly! --"
	@echo "--------------------------------------------------------"
	@echo "Using defaults:"
	@echo "\tCC=$(CC)"
	@echo "\tDDK_INCLUDE_DIR=$(DDK_INCLUDE_DIR)"
	@echo
endif
endif

check-vars:
ifeq ($(CC),$(LOCAL_MINGW64_CC))
ifneq ($(DDK_INCLUDE_DIR),$(LOCAL_MINGW64_DDK_INCLUDE_DIR))
	@echo
	@echo "------------------------------------------------------------------------"
	@echo "-- You did not set CC explicitly but set the mingw64 ddk include dir. --"
	@echo "------------------------------------------------------------------------"
	@echo "\tCC=$(CC)"
	@echo "\tDDK_INCLUDE_DIR=$(DDK_INCLUDE_DIR)"
	@echo
	@echo "This is not supported!"
	@echo
	@false
endif
endif

$(LOCAL_MINGW64_BUILD_SCRIPT):
ifeq ($(CC),$(LOCAL_MINGW64_CC))
ifeq ($(DDK_INCLUDE_DIR),$(LOCAL_MINGW64_DDK_INCLUDE_DIR))
	@echo
	@echo "------------------------------------------------------------------------------"
	@echo "-- ./mingw-w64-build/mingw-w64-build does not exist, clonging git submodule --"
	@echo "------------------------------------------------------------------------------"
	@echo
	git submodule update --init
endif
endif

$(LOCAL_MINGW64_CC):
ifeq ($(CC),$(LOCAL_MINGW64_CC))
ifeq ($(DDK_INCLUDE_DIR),$(LOCAL_MINGW64_DDK_INCLUDE_DIR))
	@echo
	@echo "----------------------------------------------------------------------------------------"
	@echo "-- ./x86_64-w64-mingw32/bin/x86_64-w64-mingw32-gcc does not exist, building toolchain --"
	@echo "----------------------------------------------------------------------------------------"
	@echo
	./mingw-w64-build/mingw-w64-build x86_64
endif
endif

.deps-built: $(LOCAL_MINGW64_BUILD_SCRIPT) $(LOCAL_MINGW64_CC)
	touch .deps-built

deps: .deps-built

$(EASTL_STATIC_LIB): .deps-built
	mkdir -p EASTL-build
	cd EASTL-build && \
		$(CMAKE) ../EASTL \
			-DCMAKE_CXX_COMPILER="$(realpath $(CXX))" \
			-DCMAKE_SYSTEM_NAME="Windows" \
			-DCMAKE_CXX_FLAGS='-ffunction-sections -fdata-sections $(CXXFLAGS) $(EASTL_CXXFLAGS)' && \
		$(MAKE) VERBOSE=1

distclean: clean
	rm -f .deps-built
	rm -rf $(LOCAL_MINGW64_BUILD_DIR)

clean:
	rm -f $(1_OBJECTS) $(1_TARGET)
	rm -f $(2_OBJECTS) $(2_TARGET)
	rm -f $(3_OBJECTS) $(3_TARGET)
	rm -f $(EASTL_COMPAT) $(EASTL_STATIC_LIB)
	$(MAKE) -C EASTL-build clean

%.o: %.c .deps-built
	$(CC) $(CFLAGS) -c $< -o $@

%.opp: %.cpp .deps-built
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(EASTL_CXXFLAGS) -c $< -o $@

$(1_TARGET): .deps-built $(1_OBJECTS)
	$(CC) -std=c99 $(CFLAGS) -Wl,--subsystem,native -Wl,--image-base,0x140000000 -Wl,--dynamicbase -Wl,--nxcompat \
		-Wl,--file-alignment,0x200 -Wl,--section-alignment,0x1000 -Wl,--stack,0x100000 \
		-Wl,--entry,DriverEntry -nostartfiles -nostdlib -o $(1_TARGET) \
		$(1_OBJECTS) -lntoskrnl -lhal

$(2_TARGET): .deps-built $(2_OBJECTS)
	$(CXX) $(CFLAGS) -Wl,--subsystem,native -Wl,--image-base,0x140000000 -Wl,--dynamicbase -Wl,--nxcompat \
		-Wl,--file-alignment,0x200 -Wl,--section-alignment,0x1000 -Wl,--stack,0x100000 \
		-Wl,--entry,DriverEntry@8 -nostartfiles -nostdlib -o $(2_TARGET) \
		$(2_OBJECTS) -lntoskrnl -lhal

$(3_TARGET): .deps-built $(EASTL_STATIC_LIB) $(EASTL_COMPAT) $(3_OBJECTS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(EASTL_CXXFLAGS) -Wl,--subsystem,native -Wl,--image-base,0x140000000 -Wl,--dynamicbase -Wl,--nxcompat \
		-Wl,--file-alignment,0x200 -Wl,--section-alignment,0x1000 -Wl,--stack,0x100000 \
		-Wl,--entry,DriverEntry@8 -nostartfiles -nostdlib -o $(3_TARGET) \
		$(3_OBJECTS) $(EASTL_COMPAT) $(EASTL_STATIC_LIB) -lntoskrnl -lhal
