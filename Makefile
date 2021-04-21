1_DRIVER_NAME = ddk-template
1_SOURCES = $(1_DRIVER_NAME).c
1_OBJECTS = $(1_DRIVER_NAME).o
1_TARGET = $(1_DRIVER_NAME).sys

2_DRIVER_NAME = ddk-template-cplusplus
2_SOURCES = $(2_DRIVER_NAME).cpp
2_OBJECTS = $(2_DRIVER_NAME).opp
2_TARGET = $(2_DRIVER_NAME).sys

3_DRIVER_NAME = ddk-template-cplusplus-EASTL
3_SOURCES = $(3_DRIVER_NAME).cpp
3_OBJECTS = $(3_DRIVER_NAME).opp
3_TARGET = $(3_DRIVER_NAME).sys

DPP_ROOT = .
INSTALL = install

all: $(1_TARGET) $(2_TARGET) $(3_TARGET)

install: all
	$(MAKE) -C $(DPP_ROOT) -f Makefile.external install-sign \
		DESTDIR=$(DESTDIR) \
		TARGETS="$(1_TARGET) $(2_TARGET) $(3_TARGET)" \
		DRIVER_DIR="$(PWD)"
	$(INSTALL) $(1_DRIVER_NAME).bat $(DESTDIR)
	$(INSTALL) $(2_DRIVER_NAME).bat $(DESTDIR)
	$(INSTALL) $(3_DRIVER_NAME).bat $(DESTDIR)

distclean: clean
	$(MAKE) -C $(DPP_ROOT) -f Makefile.external distclean

clean:
	$(MAKE) -C $(DPP_ROOT) -f Makefile.external clean
	rm -f $(1_OBJECTS) $(1_TARGET)
	rm -f $(2_OBJECTS) $(2_TARGET)
	rm -f $(3_OBJECTS) $(3_TARGET)

# simple C driver
$(1_TARGET): $(1_SOURCES)
	$(MAKE) -C $(DPP_ROOT) -f Makefile.external \
		DRIVER_TARGET="$(1_TARGET)" \
		DRIVER_DIR="$(PWD)" \
		DRIVER_OBJECTS="$(1_OBJECTS)" \
		driver-c

# C++ driver w/ MT
$(2_TARGET): $(2_SOURCES)
	$(MAKE) -C $(DPP_ROOT) -f Makefile.external \
		DRIVER_TARGET="$(2_TARGET)" \
		DRIVER_DIR="$(PWD)" \
		DRIVER_OBJECTS="$(2_OBJECTS)" \
		driver-cpp

# C++ driver w/ EASTL
$(3_TARGET): $(3_SOURCES)
	$(MAKE) -C $(DPP_ROOT) -f Makefile.external \
		DRIVER_TARGET="$(3_TARGET)" \
		DRIVER_DIR="$(PWD)" \
		DRIVER_OBJECTS="$(3_OBJECTS)" \
		driver-cpp

.PHONY: all install distclean clean
.DEFAULT_GOAL := all
