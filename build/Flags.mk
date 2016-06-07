#######################################################
# common flags
#######################################################
CXXFLAGS_COMMON += -DESCARGOT
CXXFLAGS_COMMON += -std=c++11
CXXFLAGS_COMMON += -fno-rtti -fno-math-errno -I$(ESCARGOT_ROOT)/src/
CXXFLAGS_COMMON += -fdata-sections -ffunction-sections
CXXFLAGS_COMMON += -frounding-math -fsignaling-nans
CXXFLAGS_COMMON += -Wno-invalid-offsetof
CXXFLAGS_COMMON += -DUSE_ES6_FEATURE

LDFLAGS_COMMON += -lpthread
LDFLAGS_COMMON += -lrt

#######################################################
# flags for $(HOST) : linux / tizen*
#######################################################
CXXFLAGS_LINUX += -DENABLE_CODECACHE
# CXXFLAGS_LINUX += -DENABLE_DTOACACHE

CXXFLAGS_TIZEN += -DESCARGOT_SMALL_CONFIG=1 -DESCARGOT_TIZEN
ifeq ($(MODE), release)
  CXXFLAGS_TIZEN += -Os -finline-limit=64
endif

#######################################################
# flags for $(ARCH) : x64/x86/arm
#######################################################
CXXFLAGS_X64 += -DESCARGOT_64=1
LDFLAGS_X64 =

# https://gcc.gnu.org/onlinedocs/gcc-4.8.0/gcc/i386-and-x86_002d64-Options.html
CXXFLAGS_X86 += -DESCARGOT_32=1 -m32 -mfpmath=sse -msse -msse2
LDFLAGS_X86 += -m32

CXXFLAGS_ARM += -DESCARGOT_32=1 -march=armv7-a -mthumb
LDFLAGS_ARM =

#######################################################
# flags for $(TYPE) : jit/interpreter
#######################################################
CXXFLAGS_INTERPRETER =
CXXFLAGS_JIT = -DENABLE_ESJIT=1

#######################################################
# flags for $(MODE) : debug/release
#######################################################
CXXFLAGS_DEBUG += -O0 -g3 -D_GLIBCXX_DEBUG -fno-omit-frame-pointer -Wall -Wextra -Werror
CXXFLAGS_DEBUG += -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -Wno-unused-parameter

CXXFLAGS_RELEASE += -O2 -g3 -DNDEBUG -fomit-frame-pointer -fno-stack-protector -funswitch-loops -Wno-deprecated-declarations

#######################################################
# flags for $(OUTPUT) : bin/shared_lib/static_lib
#######################################################
CXXFLAGS_BIN += -fvisibility=hidden -DESCARGOT_STANDALONE
LDFLAGS_BIN += -Wl,--gc-sections

CXXFLAGS_SHAREDLIB += -fPIC
LDFLAGS_SHAREDLIB += -ldl

CXXFLAGS_STATICLIB += -fPIC
LDFLAGS_STATICLIB += -Wl,--gc-sections

#######################################################
# flags for $(THIRD_PARTY)
#######################################################
# icu
ifeq ($(ARCH), x64)
  CXXFLAGS_THIRD_PARTY += $(shell pkg-config --cflags icu-i18n)
  LDFLAGS_THIRD_PARTY += $(shell pkg-config --libs icu-i18n)
else ifneq ($(filter $(HOST),tizen_wearable_arm tizen3_wearable_arm), )
  CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/deps/tizen/include
  LDFLAGS_THIRD_PARTY += -Ldeps/tizen/lib/tizen-wearable-$(VERSION)-target-arm
  LDFLAGS_THIRD_PARTY += -licuio -licui18n -licuuc -licudata
else ifneq ($(filter $(HOST),tizen_wearable_emulator tizen3_wearable_emulator), )
  CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/deps/tizen/include
  LDFLAGS_THIRD_PARTY += -Ldeps/tizen/lib/tizen-wearable-$(VERSION)-emulator-x86
  LDFLAGS_THIRD_PARTY += -licuio -licui18n -licuuc -licudata
else ifeq ($(ARCH), x86)
  CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/deps/x86-linux/include
  LDFLAGS_THIRD_PARTY += -Ldeps/x86-linux/lib
  LDFLAGS_THIRD_PARTY += -licuio -licui18n -licuuc -licudata
endif

# bdwgc
CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/third_party/bdwgc/include/
ifeq ($(MODE), debug)
  CXXFLAGS_THIRD_PARTY += -DGC_DEBUG
endif

# netlib
CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/third_party/netlib/

# v8's fast-dtoa
CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/third_party/double_conversion/

# rapidjson
CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/third_party/rapidjson/include/

# yarr
CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/third_party/yarr/

#######################################################
# for printing TC coverage log
#######################################################
CXXFLAGS_TC = -DSTARFISH_TC_COVERAGE

