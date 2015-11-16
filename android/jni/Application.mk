ifeq ($(BUILD_OBJECT), )
	BUILD_OBJECT = exe
endif

ifeq ($(BUILD_MODE), )
	BUILD_MODE = release
endif

ifeq ($(BUILD_ARCH), )
	BUILD_ARCH = all
endif

ifeq ($(REACT_NATIVE), 1)
	BUILD_ARCH = armeabi-v7a x86
endif

ifeq ($(TOOL_CHAIN_VERSION), )
	TOOL_CHAIN_VERSION = 4.9
endif

NDK_TOOLCHAIN_VERSION=$(TOOL_CHAIN_VERSION)
APP_OPTIM := $(BUILD_MODE)
APP_ABI := $(BUILD_ARCH)
APP_PLATFORM := android-19
APP_STL := gnustl_static
#APP_STL := stlport_static
APP_CFLAGS := -fexceptions
