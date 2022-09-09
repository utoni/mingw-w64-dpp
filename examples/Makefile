ifndef DPP_ROOT
$(error DPP_ROOT is undefined, use `make DPP_ROOT=$(realpath ..)`)
endif

include $(DPP_ROOT)/Makefile.inc

1_DRIVER_NAME = dpp-example
1_OBJECTS = $(1_DRIVER_NAME).o
1_TARGET = $(1_DRIVER_NAME).sys

2_DRIVER_NAME = dpp-example-cplusplus
2_OBJECTS = $(2_DRIVER_NAME).opp
2_TARGET = $(2_DRIVER_NAME).sys

3_DRIVER_NAME = dpp-example-cplusplus-EASTL
3_OBJECTS = $(3_DRIVER_NAME).opp
3_TARGET = $(3_DRIVER_NAME).sys

ifdef BUILD_NATIVE
$(error There is nothing the build as native executable here..)
endif

all: $(1_TARGET) $(2_TARGET) $(3_TARGET)

include $(DPP_ROOT)/Makefile.inc

%.o: %.c
	$(call BUILD_C_OBJECT,$<,$@)

%.opp: %.cpp
	$(call BUILD_CPP_OBJECT,$<,$@)

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
	$(INSTALL) $(1_DRIVER_NAME).bat $(DESTDIR)
	$(INSTALL) $(2_DRIVER_NAME).bat $(DESTDIR)
	$(INSTALL) $(3_DRIVER_NAME).bat $(DESTDIR)

clean:
	rm -f $(1_OBJECTS) $(1_TARGET) $(1_TARGET).map
	rm -f $(2_OBJECTS) $(2_TARGET) $(2_TARGET).map
	rm -f $(3_OBJECTS) $(3_TARGET) $(3_TARGET).map

.PHONY: all install clean
.DEFAULT_GOAL := all
