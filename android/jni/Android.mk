LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#$(warning BUILD_TYPE)
#$(warning $(BUILD_TYPE))
#$(warning BUILD_MODE)
#$(warning $(BUILD_OBJECT))

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
	LOCAL_ARM_NEON := true
else ifeq ($(TARGET_ARCH), x86)
	LOCAL_CFLAGS += -DESCARGOT_32=1
else ifeq ($(TARGET_ARCH), mips)
	LOCAL_CFLAGS += -DESCARGOT_32=1
endif


LOCAL_CFLAGS += -fno-math-errno -I$(SRC_PATH)
LOCAL_CFLAGS += -fdata-sections -ffunction-sections -frounding-math -fsignaling-nans

ifeq ($(REACT_NATIVE), )
else
	LOCAL_CXXFLAGS += -frtti
endif

ifeq ($(BUILD_MODE), debug)
    LOCAL_CFLAGS += -O0 -g3 -D_GLIBCXX_DEBUG -fno-omit-frame-pointer -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-sign-compare -Wno-unused-local-typedefs
    LOCAL_CXXFLAGS += -Wno-invalid-offsetof
else ifeq ($(BUILD_MODE), release)
    LOCAL_CFLAGS += -O2 -g0 -DNDEBUG -fomit-frame-pointer -finline-limit=300 -fno-stack-protector -funswitch-loops
else
    $(error mode error)
endif

SRC_PATH=../src
SRC_THIRD_PARTY=../third_party

SRCS =

ifeq ($(BUILD_TYPE), jit)

	####################################
	# ARCH-dependent settings
	####################################

	ifeq ($(TARGET_ARCH), x64)
		TARGET_CPU=x86_64
		LOCAL_CXXFLAGS += -DAVMPLUS_64BIT
		LOCAL_CXXFLAGS += -DAVMPLUS_AMD64
		LOCAL_CXXFLAGS += #if defined(_M_AMD64) || defined(_M_X64)
		SRCS += $(SRC_THIRD_PARTY)/nanojit/NativeX64.cpp
	else ifeq ($(TARGET_ARCH), x86)
		TARGET_CPU=i686
		LOCAL_CXXFLAGS += -DAVMPLUS_32BIT
		LOCAL_CXXFLAGS += -DAVMPLUS_IA32
		LOCAL_CXXFLAGS += #if defined(_M_AMD64) || defined(_M_X64)
		SRCS += $(SRC_THIRD_PARTY)/nanojit/Nativei386.cpp
	else ifeq($(TARGET_ARCH), arm)
		TARGET_CPU=arm
#		LOCAL_CXXFLAGS += -mfpu=neon #enabled by LOCAL_ARM_NEON := true
		LOCAL_CXXFLAGS += -DAVMPLUS_32BIT
		LOCAL_CXXFLAGS += -DAVMPLUS_ARM
		LOCAL_CXXFLAGS += -DTARGET_THUMB2
		LOCAL_CXXFLAGS += #if defined(_M_AMD64) || defined(_M_X64)
		SRCS += $(SRC_THIRD_PARTY)/nanojit/NativeARM.cpp
		SRCS += $(SRC_THIRD_PARTY)/nanojit/NativeThumb2.cpp
	endif

	####################################
	# target-dependent settings
	####################################

	ifeq ($(BUILD_MODE), debug)
		LOCAL_CXXFLAGS += -DDEBUG
		LOCAL_CXXFLAGS += -D_DEBUG
		LOCAL_CXXFLAGS += -DNJ_VERBOSE
		LOCAL_CXXFLAGS += -Wno-error=narrowing
	endif

	####################################
	# Other features
	####################################

	LOCAL_CXXFLAGS += -I$(SRC_THIRD_PARTY)/nanojit/
	LOCAL_CXXFLAGS += -DESCARGOT
    LOCAL_CXXFLAGS += -DENABLE_ESJIT=1
	LOCAL_CXXFLAGS += -DFEATURE_NANOJIT
    LOCAL_CXXFLAGS += -DAVMPLUS_UNIX=1
    LOCAL_CXXFLAGS += -DANDROID=1

endif


LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/rapidjson/include/
LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/yarr/
LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/double_conversion/
LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/netlib/
ifeq ($(REACT_NATIVE), )
    LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/bdwgc/include/ -DHAVE_CONFIG_H -I$(SRC_THIRD_PARTY)/bdwgc/android/include/ -I$(SRC_THIRD_PARTY)/bdwgc/libatomic_ops/src
else
    LOCAL_CFLAGS += -I$(SRC_THIRD_PARTY)/bdwgc/include/ -DHAVE_CONFIG_H -I$(SRC_THIRD_PARTY)/bdwgc/android_multithreaded/include/ -I$(SRC_THIRD_PARTY)/bdwgc/libatomic_ops/src   -DGC_THREADS -DGC_PTHREADS -DTHREADS -DLINUX
endif

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += -Wl,--gc-sections

SRCS += $(foreach dir, $(SRC_PATH)/ast , $(wildcard $(dir)/*.cpp))
SRCS += $(foreach dir, $(SRC_PATH)/bytecode , $(wildcard $(dir)/*.cpp))
SRCS += $(foreach dir, $(SRC_PATH)/jit , $(wildcard $(dir)/*.cpp))
SRCS += $(foreach dir, $(SRC_PATH)/parser , $(wildcard $(dir)/*.cpp))
SRCS += $(foreach dir, $(SRC_PATH)/runtime , $(wildcard $(dir)/*.cpp))
ifeq ($(BUILD_OBJECT), exe)
	SRCS += $(foreach dir, $(SRC_PATH)/shell , $(wildcard $(dir)/*.cpp))
	LOCAL_CFLAGS += -fvisibility=hidden
endif
SRCS += $(foreach dir, $(SRC_PATH)/vm , $(wildcard $(dir)/*.cpp))

SRCS += $(SRC_THIRD_PARTY)/yarr/OSAllocatorPosix.cpp
SRCS += $(SRC_THIRD_PARTY)/yarr/PageBlock.cpp
SRCS += $(SRC_THIRD_PARTY)/yarr/YarrCanonicalizeUCS2.cpp
SRCS += $(SRC_THIRD_PARTY)/yarr/YarrInterpreter.cpp
SRCS += $(SRC_THIRD_PARTY)/yarr/YarrPattern.cpp
SRCS += $(SRC_THIRD_PARTY)/yarr/YarrSyntaxChecker.cpp

ifeq ($(BUILD_TYPE), jit)
	SRCS += $(SRC_THIRD_PARTY)/nanojit/Allocator.cpp
	SRCS += $(SRC_THIRD_PARTY)/nanojit/Assembler.cpp
	SRCS += $(SRC_THIRD_PARTY)/nanojit/CodeAlloc.cpp
	SRCS += $(SRC_THIRD_PARTY)/nanojit/Containers.cpp
	SRCS += $(SRC_THIRD_PARTY)/nanojit/Fragmento.cpp
	SRCS += $(SRC_THIRD_PARTY)/nanojit/LIR.cpp
	SRCS += $(SRC_THIRD_PARTY)/nanojit/njconfig.cpp
	SRCS += $(SRC_THIRD_PARTY)/nanojit/RegAlloc.cpp
	SRCS += $(SRC_THIRD_PARTY)/nanojit/EscargotBridge.cpp
endif

SRCS += $(foreach dir, $(SRC_THIRD_PARTY)/double_conversion , $(wildcard $(dir)/*.cc))

SRCS += $(foreach dir, $(SRC_THIRD_PARTY)/bdwgc , $(wildcard $(dir)/*.c))
SRCS += $(foreach dir, $(SRC_THIRD_PARTY)/bdwgc/libatomic_ops/src , $(wildcard $(dir)/*.c))

LOCAL_SRC_FILES += $(addprefix ../, $(SRCS))
ifeq ($(BUILD_OBJECT), exe)
	include $(BUILD_EXECUTABLE)
else
	include $(BUILD_SHARED_LIBRARY)
endif
