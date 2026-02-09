ifndef DPP_ROOT
$(error DPP_ROOT is undefined, use `make DPP_ROOT=$(realpath ..)`)
endif

ifndef BUILD_NATIVE
include $(DPP_ROOT)/Makefile.inc
OPP_SUFFIX := kopp
EXE_SUFFIX := sys
else
include $(DPP_ROOT)/Makefile.native.inc
ifdef DEBUG
CFLAGS += -g -fsanitize=address -fsanitize=leak
endif
OPP_SUFFIX := uopp
EXE_SUFFIX := run
endif

1_DRIVER_NAME = dpp-example
1_OBJECTS = $(1_DRIVER_NAME).o
1_TARGET = $(1_DRIVER_NAME).$(EXE_SUFFIX)

2_DRIVER_NAME = dpp-example-cplusplus
2_OBJECTS = $(2_DRIVER_NAME).$(OPP_SUFFIX)
2_TARGET = $(2_DRIVER_NAME).$(EXE_SUFFIX)

3_DRIVER_NAME = dpp-example-cplusplus-EASTL
3_OBJECTS = $(3_DRIVER_NAME).$(OPP_SUFFIX)
3_TARGET = $(3_DRIVER_NAME).$(EXE_SUFFIX)

ifndef BUILD_NATIVE
all: $(1_TARGET) $(2_TARGET) $(3_TARGET)
else
all: $(3_TARGET)
endif

%.o: %.c
	$(call BUILD_C_OBJECT,$<,$@)

%.$(OPP_SUFFIX): %.cpp
	$(call BUILD_CPP_OBJECT,$<,$@)

ifndef BUILD_NATIVE
# simple C driver
$(1_TARGET): $(1_OBJECTS)
	$(call LINK_C_KERNEL_TARGET,$(1_OBJECTS),$@)

# C++ driver w/ MT
$(2_TARGET): $(2_OBJECTS)
	$(call LINK_CPP_KERNEL_TARGET,$(2_OBJECTS),$@)

# C++ driver w/ EASTL
$(3_TARGET): $(3_OBJECTS)
	$(call LINK_CPP_KERNEL_TARGET,$(3_OBJECTS),$@)

install: all
	$(call INSTALL_EXEC_SIGN,$(1_TARGET))
	$(call INSTALL_EXEC_SIGN,$(2_TARGET))
	$(call INSTALL_EXEC_SIGN,$(3_TARGET))
	$(CP) $(1_DRIVER_NAME).bat $(DESTDIR)
	$(CP) $(2_DRIVER_NAME).bat $(DESTDIR)
	$(CP) $(3_DRIVER_NAME).bat $(DESTDIR)
else
$(1_TARGET):
	$(error Target $(1_TARGET) does not support BUILD_NATIVE)

# C++ driver w/ MT
$(2_TARGET):
	$(error Target $(2_TARGET) does not support BUILD_NATIVE)

# C++ driver w/ EASTL
$(3_TARGET): $(3_OBJECTS)
	$(call LINK_CPP_USER_TARGET,$(3_OBJECTS),$@)
endif

$(1_DRIVER_NAME): $(1_TARGET)
	@echo 'Target compiled/linked: $(1_TARGET)'
$(2_DRIVER_NAME): $(2_TARGET)
	@echo 'Target compiled/linked: $(2_TARGET)'
$(3_DRIVER_NAME): $(3_TARGET)
	@echo 'Target compiled/linked: $(3_TARGET)'

clean:
	rm -f $(1_OBJECTS) $(1_TARGET) $(1_TARGET).map
	rm -f $(2_OBJECTS) $(2_TARGET) $(2_TARGET).map
	rm -f $(3_OBJECTS) $(3_TARGET) $(3_TARGET).map

.PHONY: all install clean
.DEFAULT_GOAL := all
