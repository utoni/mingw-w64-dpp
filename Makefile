LOCAL_MINGW64_BUILD_SCRIPT := ./mingw-w64-build/mingw-w64-build
LOCAL_MINGW64_BUILD_DIR := ./x86_64-w64-mingw32
LOCAL_MINGW64_CC := $(LOCAL_MINGW64_BUILD_DIR)/bin/x86_64-w64-mingw32-gcc
LOCAL_MINGW64_DDK_INCLUDE_DIR := $(LOCAL_MINGW64_BUILD_DIR)/x86_64-w64-mingw32/include/ddk

CC = $(LOCAL_MINGW64_CC)
CXX = $(dir $(CC))/x86_64-w64-mingw32-g++
DDK_INCLUDE_DIR = $(LOCAL_MINGW64_DDK_INCLUDE_DIR)
CFLAGS = -Wall -m64 -shared \
	-I$(DDK_INCLUDE_DIR) \
	-D__INTRINSIC_DEFINED_InterlockedBitTestAndSet \
	-D__INTRINSIC_DEFINED_InterlockedBitTestAndReset

1_DRIVER_NAME = ddk-template
1_OBJECTS = $(1_DRIVER_NAME).o
1_TARGET = $(1_DRIVER_NAME).sys

2_DRIVER_NAME = ddk-template-cplusplus
2_OBJECTS = $(2_DRIVER_NAME).opp
2_TARGET = $(2_DRIVER_NAME).sys

all: deps-print-local-notice check-vars $(1_TARGET) $(2_TARGET)

deps-print-local-notice:
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

clean:
	rm -f $(1_OBJECTS) $(1_TARGET)
	rm -f $(2_OBJECTS) $(2_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.opp: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

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
