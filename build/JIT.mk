#include third_party/nanojit/Build.mk
####################################
# ARCH-dependent settings
####################################
ifeq ($(ARCH), x64)
TARGET_CPU=x86_64
CXXFLAGS += -DAVMPLUS_64BIT
CXXFLAGS += -DAVMPLUS_AMD64
CXXFLAGS += #if defined(_M_AMD64) || defined(_M_X64)
else ifeq ($(ARCH), x86)
TARGET_CPU=i686
CXXFLAGS += -DAVMPLUS_32BIT
CXXFLAGS += -DAVMPLUS_IA32
CXXFLAGS += #if defined(_M_AMD64) || defined(_M_X64)
else ifeq ($(ARCH), arm)
TARGET_CPU=arm
# CXXFLAGS += -mfpu=neon #enabled by LOCAL_ARM_NEON := true
CXXFLAGS += -DAVMPLUS_32BIT
CXXFLAGS += -DAVMPLUS_ARM
CXXFLAGS += -DTARGET_THUMB2
CXXFLAGS += #if defined(_M_AMD64) || defined(_M_X64)
SRCS += $(SRC_THIRD_PARTY)/nanojit/NativeARM.cpp
SRCS += $(SRC_THIRD_PARTY)/nanojit/NativeThumb2.cpp
endif

####################################
# target-dependent settings
####################################

ifeq ($(MODE), debug)
CXXFLAGS += -DDEBUG
CXXFLAGS += -D_DEBUG
CXXFLAGS += -DNJ_VERBOSE
endif

####################################
# Other features
####################################
CXXFLAGS += -DESCARGOT
CXXFLAGS += -Ithird_party/nanojit/
CXXFLAGS += -DFEATURE_NANOJIT

#CXXFLAGS += -DAVMPLUS_VERBOSE
CXXFLAGS += -Wno-error=narrowing

####################################
# Makefile flags
####################################
curdir=third_party/nanojit
include $(curdir)/manifest.mk
SRC_NANOJIT = $(avmplus_CXXSRCS)
SRC_NANOJIT += $(curdir)/EscargotBridge.cpp

SRC += $(SRC_NANOJIT)
