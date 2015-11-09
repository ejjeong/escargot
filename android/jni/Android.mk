LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#$(warning BUILD_TYPE)
#$(warning $(BUILD_TYPE))
#$(warning BUILD_MODE)
#$(warning $(BUILD_MODE))

LOCAL_MODULE    := escargot
LOCAL_ARM_MODE := thumb
LOCAL_CXXFLAGS = -std=c++11 -fno-rtti
LOCAL_CFLAGS =

$(warning TARGET_ARCH)
$(warning $(TARGET_ARCH))
 

ifeq ($(TARGET_ARCH), arm64)
	LOCAL_CFLAGS += -DESCARGOT_64=1
else ifeq ($(TARGET_ARCH), x86_64)
	LOCAL_CFLAGS += -DESCARGOT_64=1
else ifeq ($(TARGET_ARCH), mips64)
	LOCAL_CFLAGS += -DESCARGOT_64=1
else ifeq ($(TARGET_ARCH), arm)
	LOCAL_CFLAGS += -DESCARGOT_32=1
else ifeq ($(TARGET_ARCH), x86)
	LOCAL_CFLAGS += -DESCARGOT_32=1
else ifeq ($(TARGET_ARCH), mips)
	LOCAL_CFLAGS += -DESCARGOT_32=1
endif


LOCAL_CFLAGS += -fno-rtti -fno-math-errno -I$(SRC_PATH)
LOCAL_CFLAGS += -fdata-sections -ffunction-sections -frounding-math -fsignaling-nans

ifeq ($(BUILD_MODE), debug)
    LOCAL_CFLAGS += -O0 -g3 -D_GLIBCXX_DEBUG -fno-omit-frame-pointer -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-sign-compare -Wno-unused-local-typedefs
    LOCAL_CXXFLAGS += -Wno-invalid-offsetof
else ifeq ($(BUILD_MODE), release)
    LOCAL_CFLAGS += -O2 -g0 -DNDEBUG -fomit-frame-pointer
else
    $(error mode error)
endif

SRC_PATH=../src
SRC_THIRD_PARTY=../third_party

LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/rapidjson/include/
LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/yarr/
LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/double_conversion/
LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/netlib/
LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/bdwgc/include/ -DHAVE_CONFIG_H -I$(SRC_THIRD_PARTY)/bdwgc/android/include/ -I$(SRC_THIRD_PARTY)/bdwgc/libatomic_ops/src 

LOCAL_STATIC_LIBRARIES := gc
LOCAL_SHARED_LIBRARIES += -lpthread
LOCAL_LDFLAGS += -Wl,--gc-sections

SRCS = $(foreach dir, $(SRC_PATH)/ast , $(wildcard $(dir)/*.cpp))
SRCS += $(foreach dir, $(SRC_PATH)/bytecode , $(wildcard $(dir)/*.cpp))
SRCS += $(foreach dir, $(SRC_PATH)/jit , $(wildcard $(dir)/*.cpp))
SRCS += $(foreach dir, $(SRC_PATH)/parser , $(wildcard $(dir)/*.cpp))
SRCS += $(foreach dir, $(SRC_PATH)/runtime , $(wildcard $(dir)/*.cpp))
SRCS += $(foreach dir, $(SRC_PATH)/shell , $(wildcard $(dir)/*.cpp))
SRCS += $(foreach dir, $(SRC_PATH)/vm , $(wildcard $(dir)/*.cpp))

SRCS += $(SRC_THIRD_PARTY)/yarr/OSAllocatorPosix.cpp
SRCS += $(SRC_THIRD_PARTY)/yarr/PageBlock.cpp
SRCS += $(SRC_THIRD_PARTY)/yarr/YarrCanonicalizeUCS2.cpp
SRCS += $(SRC_THIRD_PARTY)/yarr/YarrInterpreter.cpp
SRCS += $(SRC_THIRD_PARTY)/yarr/YarrPattern.cpp
SRCS += $(SRC_THIRD_PARTY)/yarr/YarrSyntaxChecker.cpp

SRCS += $(foreach dir, $(SRC_THIRD_PARTY)/double_conversion , $(wildcard $(dir)/*.cc))

SRCS += $(foreach dir, $(SRC_THIRD_PARTY)/bdwgc , $(wildcard $(dir)/*.c))

LOCAL_SRC_FILES += $(addprefix ../, $(SRCS))

include $(BUILD_EXECUTABLE)