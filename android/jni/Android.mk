LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := gc
LOCAL_SRC_FILES := libgc_arm.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

$(warning BUILD_TYPE)
$(warning $(BUILD_TYPE))
$(warning BUILD_MODE)
$(warning $(BUILD_MODE))

LOCAL_MODULE    := escargot
LOCAL_ARM_NEON := true
LOCAL_CFLAGS = -std=c++11 -fno-rtti

ifeq ($(BUILD_MODE), debug)
        LOCAL_CFLAGS += -O0 -g3 -D_GLIBCXX_DEBUG -frounding-math -fsignaling-nans -fno-omit-frame-pointer -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-invalid-offsetof -Wno-sign-compare -Wno-unused-local-typedefs
else ifeq ($(BUILD_MODE), release)
        LOCAL_CFLAGS += -O2 -g3 -DNDEBUG -fomit-frame-pointer -frounding-math -fsignaling-nans
else
        $(error mode error)
endif

LOCAL_CFLAGS += -DESCARGOT_32=1
LOCAL_CFLAGS += -fno-rtti -fno-math-errno -I$(SRC_PATH)
LOCAL_CFLAGS += -fdata-sections -ffunction-sections

SRC_PATH=../src
SRC_THIRD_PARTY=../third_party

LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/rapidjson/include/
LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/yarr/
LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/double_conversion/
LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/netlib/
LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/bdwgc/include/

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

SRCS += $(SRC_THIRD_PARTY)/double_conversion/fast-dtoa.cpp
SRCS += $(SRC_THIRD_PARTY)/double_conversion/diy-fp.cpp
SRCS += $(SRC_THIRD_PARTY)/double_conversion/cached-powers.cpp
SRCS += $(SRC_THIRD_PARTY)/double_conversion/bignum.cpp
SRCS += $(SRC_THIRD_PARTY)/double_conversion/bignum-dtoa.cpp

LOCAL_SRC_FILES += $(addprefix ../, $(SRCS))

include $(BUILD_EXECUTABLE)