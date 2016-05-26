ifeq ($(HOST), linux)
  CC           = gcc
  CXX          = g++
  STRIP        = strip
else ifneq (,$(findstring tizen,$(HOST)))

  ifneq (,$(findstring tizen_,$(HOST)))
    ifndef TIZEN_SDK_HOME
      $(error TIZEN_SDK_HOME must be set)
    endif
    VERSION=2.3.1
    TIZEN_ROOT=$(TIZEN_SDK_HOME)
  else ifneq (,$(findstring tizen3_,$(HOST)))
    ifndef TIZEN3_SDK_HOME
      $(error TIZEN3_SDK_HOME must be set)
    endif
    VERSION=3.0
    TIZEN_ROOT=$(TIZEN3_SDK_HOME)
  endif

  ifneq (,$(findstring mobile,$(HOST)))
    ifeq ($(ARCH), arm)
      TIZEN_SYSROOT=$(TIZEN_ROOT)/platforms/tizen-$(VERSION)/mobile/rootstraps/mobile-$(VERSION)-device.core
      COMPILER_PREFIX=arm-linux-gnueabi
    endif
  else ifneq (,$(findstring wearable,$(HOST)))
    ifeq ($(ARCH), arm)
      TIZEN_SYSROOT=$(TIZEN_ROOT)/platforms/tizen-$(VERSION)/wearable/rootstraps/wearable-$(VERSION)-device.core
      COMPILER_PREFIX=arm-linux-gnueabi
    else ifeq ($(ARCH), x86)
      TIZEN_SYSROOT=$(TIZEN_ROOT)/platforms/tizen-$(VERSION)/wearable/rootstraps/wearable-$(VERSION)-emulator.core
      COMPILER_PREFIX=i386-linux-gnueabi
    endif
  endif

  CC    = $(TIZEN_ROOT)/tools/$(COMPILER_PREFIX)-gcc-4.9/bin/$(COMPILER_PREFIX)-gcc
  CXX   = $(TIZEN_ROOT)/tools/$(COMPILER_PREFIX)-gcc-4.9/bin/$(COMPILER_PREFIX)-g++
  LINK  = $(TIZEN_ROOT)/tools/$(COMPILER_PREFIX)-gcc-4.9/bin/$(COMPILER_PREFIX)-g++
  LD    = $(TIZEN_ROOT)/tools/$(COMPILER_PREFIX)-gcc-4.9/bin/$(COMPILER_PREFIX)-ld
  AR    = $(TIZEN_ROOT)/tools/$(COMPILER_PREFIX)-gcc-4.9/bin/$(COMPILER_PREFIX)-ar
  STRIP = $(TIZEN_ROOT)/tools/$(COMPILER_PREFIX)-gcc-4.9/bin/$(COMPILER_PREFIX)-strip

  CXXFLAGS += --sysroot=$(TIZEN_SYSROOT)
  LDFLAGS  += --sysroot=$(TIZEN_SYSROOT)

endif
