ifeq ($(HOST), linux)
  CC           = gcc
  CXX          = g++
  ARFLAGS      =
else ifeq ($(HOST), tizen_obs)
  CC=gcc
  CXX=g++
  CXXFLAGS += $(shell pkg-config --cflags dlog)
  LDFLAGS += $(shell pkg-config --libs dlog)
  ifeq ($(LTO), 1)
    ARFLAGS=--plugin=/usr/lib/bfd-plugins/liblto_plugin.so
  else
    ARFLAGS=
  endif
else ifneq (,$(findstring tizen,$(HOST)))
  ifndef TIZEN_SDK_HOME
    $(error TIZEN_SDK_HOME must be set)
  endif

  ifneq (,$(findstring mobile,$(HOST)))
    ifeq ($(ARCH), arm)
      TIZEN_SYSROOT=$(TIZEN_SDK_HOME)/platforms/tizen-$(VERSION)/mobile/rootstraps/mobile-$(VERSION)-device.core
    endif
  else ifneq (,$(findstring wearable,$(HOST)))
    ifeq ($(ARCH), arm)
      TIZEN_SYSROOT=$(TIZEN_SDK_HOME)/platforms/tizen-$(VERSION)/wearable/rootstraps/wearable-$(VERSION)-device.core
    else ifeq ($(ARCH), i386)
      TIZEN_SYSROOT=$(TIZEN_SDK_HOME)/platforms/tizen-$(VERSION)/wearable/rootstraps/wearable-$(VERSION)-emulator.core
    endif
  endif

  COMPILER_PREFIX=$(ARCH)-linux-gnueabi
  CC    = $(TIZEN_SDK_HOME)/tools/$(COMPILER_PREFIX)-gcc-4.6/bin/$(COMPILER_PREFIX)-gcc
  CXX   = $(TIZEN_SDK_HOME)/tools/$(COMPILER_PREFIX)-gcc-4.6/bin/$(COMPILER_PREFIX)-g++
  LINK  = $(TIZEN_SDK_HOME)/tools/$(COMPILER_PREFIX)-gcc-4.6/bin/$(COMPILER_PREFIX)-g++
  LD    = $(TIZEN_SDK_HOME)/tools/$(COMPILER_PREFIX)-gcc-4.6/bin/$(COMPILER_PREFIX)-ld
  ifeq ($(LTO), 1)
    AR    = $(TIZEN_SDK_HOME)/tools/$(COMPILER_PREFIX)-gcc-4.6/bin/$(COMPILER_PREFIX)-gcc-ar
  else
    AR    = $(TIZEN_SDK_HOME)/tools/$(COMPILER_PREFIX)-gcc-4.6/bin/$(COMPILER_PREFIX)-ar
  endif

  CXXFLAGS += --sysroot=$(TIZEN_SYSROOT)
  LDFLAGS  += --sysroot=$(TIZEN_SYSROOT)
  ifeq ($(LTO), 1)
    ARFLAGS  = --plugin=$(TIZEN_SDK_HOME)/tools/$(COMPILER_PREFIX)-gcc-4.6/libexec/gcc/$(COMPILER_PREFIX)/4.6.4/liblto_plugin.so
  else
    ARFLAGS  =
  endif
endif
