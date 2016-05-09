ifeq ($(HOST), linux)
  CC           = gcc
  CXX          = g++
  STRIP        = strip
  CXXFLAGS     = -std=c++11
else ifneq ($(filter $(HOST),tizen_arm tizen3_arm), )
  ifeq ($(HOST), tizen_arm)
    ifndef TIZEN_SDK_HOME
      $(error TIZEN_SDK_HOME must be set)
    endif
    VERSION=2.4
    TIZEN_ROOT=$(TIZEN_SDK_HOME)
  endif
  ifeq ($(HOST), tizen3_arm)
    ifndef TIZEN3_SDK_HOME
      $(error TIZEN3_SDK_HOME must be set)
    endif
    VERSION=3.0
    TIZEN_ROOT=$(TIZEN3_SDK_HOME)
  endif
  TIZEN_TOOLCHAIN=$(TIZEN_ROOT)/tools/arm-linux-gnueabi-gcc-4.9
  TIZEN_SYSROOT=$(TIZEN_ROOT)/platforms/tizen-$(VERSION)/mobile/rootstraps/mobile-$(VERSION)-device.core
  # Setting For Tizen 2.3 SDK
  # TIZEN_TOOLCHAIN=$(TIZEN_ROOT)/tools/arm-linux-gnueabi-gcc-4.6
  # TIZEN_SYSROOT=/home/chokobole/Workspace/Tizen-Sdk/2.3/platforms/mobile-2.3/rootstraps/mobile-2.3-device.core
  CC    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-gcc
  CXX   = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-g++
  LINK  = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-g++
  LD    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-ld
  AR    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-ar
  STRIP = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-strip
  CXXFLAGS     += -std=c++11 --sysroot=$(TIZEN_SYSROOT) -mthumb
  LDFLAGS     += --sysroot=$(TIZEN_SYSROOT)
else ifneq ($(filter $(HOST),tizen_wearable_arm tizen3_wearable_arm), )
  ifeq ($(HOST), tizen_wearable_arm)
    ifndef TIZEN_SDK_HOME
      $(error TIZEN_SDK_HOME must be set)
    endif
    VERSION=2.3.1
    TIZEN_ROOT=$(TIZEN_SDK_HOME)
  endif
  ifeq ($(HOST), tizen3_wearable_arm)
    ifndef TIZEN3_SDK_HOME
      $(error TIZEN3_SDK_HOME must be set)
    endif
    VERSION=3.0
    TIZEN_ROOT=$(TIZEN3_SDK_HOME)
  endif

  TIZEN_TOOLCHAIN=$(TIZEN_ROOT)/tools/arm-linux-gnueabi-gcc-4.9
  TIZEN_SYSROOT=$(TIZEN_ROOT)/platforms/tizen-$(VERSION)/wearable/rootstraps/wearable-$(VERSION)-device.core
  CC    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-gcc
  CXX   = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-g++
  LINK  = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-g++
  LD    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-ld
  AR    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-ar
  STRIP = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-strip
  CXXFLAGS     += -std=c++11 --sysroot=$(TIZEN_SYSROOT) -DESCARGOT_SMALL_CONFIG=1 -mthumb -finline-limit=64 -DESCARGOT_TIZEN -I$(TIZEN_SYSROOT)/usr/include
  LDFLAGS     += --sysroot=$(TIZEN_SYSROOT) -ldlog -L$(TIZEN_SYSROOT)/usr/lib
else
  ifeq ($(HOST), tizen_wearable_emulator)
    ifndef TIZEN_SDK_HOME
      $(error TIZEN_SDK_HOME must be set)
    endif
    VERSION=2.3.1
    TIZEN_ROOT=$(TIZEN_SDK_HOME)
  endif
  ifeq ($(HOST), tizen3_wearable_emulator)
    ifndef TIZEN3_SDK_HOME
      $(error TIZEN3_SDK_HOME must be set)
    endif
    VERSION=3.0
    TIZEN_ROOT=$(TIZEN3_SDK_HOME)
  endif

  TIZEN_TOOLCHAIN=$(TIZEN_ROOT)/tools/i386-linux-gnueabi-gcc-4.9
  TIZEN_SYSROOT=$(TIZEN_ROOT)/platforms/tizen-$(VERSION)/wearable/rootstraps/wearable-$(VERSION)-emulator.core
  CC    = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-gcc
  CXX   = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-g++
  LINK  = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-g++
  LD    = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-ld
  AR    = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-ar
  STRIP = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-strip
  CXXFLAGS     += -std=c++11 --sysroot=$(TIZEN_SYSROOT) -DESCARGOT_SMALL_CONFIG=1 -finline-limit=64  -DESCARGOT_TIZEN
  LDFLAGS     += --sysroot=$(TIZEN_SYSROOT)
endif
