ifeq ($(BUILD_MODE), )
	BUILD_MODE = release
endif

ifeq ($(BUILD_ARCH), )
	BUILD_ARCH = all
endif

NDK_TOOLCHAIN_VERSION=4.9
APP_OPTIM := $(BUILD_MODE)
APP_ABI := $(BUILD_ARCH)
APP_PLATFORM := android-19
APP_STL := gnustl_static
#APP_STL := stlport_static
APP_CFLAGS := -fexceptions