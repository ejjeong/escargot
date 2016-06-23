#######################################################
# common flags
#######################################################
ESCARGOT_CXXFLAGS_COMMON += -DESCARGOT
ESCARGOT_CXXFLAGS_COMMON += -std=c++0x
ESCARGOT_CXXFLAGS_COMMON += -fno-rtti -fno-math-errno -I$(ESCARGOT_ROOT)/src/
ESCARGOT_CXXFLAGS_COMMON += -fdata-sections -ffunction-sections
ESCARGOT_CXXFLAGS_COMMON += -frounding-math -fsignaling-nans
ESCARGOT_CXXFLAGS_COMMON += -DUSE_ES6_FEATURE

ESCARGOT_LDFLAGS_COMMON += -lpthread
ESCARGOT_LDFLAGS_COMMON += -lrt

#######################################################
# flags for $(HOST) : linux / tizen*
#######################################################
ESCARGOT_CXXFLAGS_LINUX += -DENABLE_CODECACHE
# ESCARGOT_CXXFLAGS_LINUX += -DENABLE_DTOACACHE

ESCARGOT_CXXFLAGS_TIZEN += -DESCARGOT_SMALL_CONFIG=1 -DESCARGOT_TIZEN

#######################################################
# flags for $(ARCH) : x64/x86/arm
#######################################################
ESCARGOT_CXXFLAGS_X64 += -DESCARGOT_64=1
ESCARGOT_LDFLAGS_X64 =

# https://gcc.gnu.org/onlinedocs/gcc-4.8.0/gcc/i386-and-x86_002d64-Options.html
ESCARGOT_CXXFLAGS_X86 += -DESCARGOT_32=1 -m32 -mfpmath=sse -msse -msse2
ESCARGOT_LDFLAGS_X86 += -m32

ESCARGOT_CXXFLAGS_ARM += -DESCARGOT_32=1 -march=armv7-a -mthumb
ESCARGOT_LDFLAGS_ARM =

#######################################################
# flags for $(TYPE) : jit/interpreter
#######################################################
ESCARGOT_CXXFLAGS_INTERPRETER =
ESCARGOT_CXXFLAGS_JIT = -DENABLE_ESJIT=1
ESCARGOT_CXXFLAGS_JIT += -Wno-invalid-offsetof

#######################################################
# flags for $(MODE) : debug/release
#######################################################
ESCARGOT_CXXFLAGS_DEBUG += -O0 -g3 -D_GLIBCXX_DEBUG -fno-omit-frame-pointer -Wall -Wextra -Werror
ESCARGOT_CXXFLAGS_DEBUG += -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -Wno-unused-parameter
ESCARGOT_CXXFLAGS_DEBUG += -Wno-type-limits -Wno-unused-result # TODO: enable these warnings

ESCARGOT_CXXFLAGS_RELEASE += -O2 -g3 -DNDEBUG -fomit-frame-pointer -fno-stack-protector -funswitch-loops -Wno-deprecated-declarations
ifneq (,$(findstring tizen,$(HOST)))
  ESCARGOT_CXXFLAGS_RELEASE += -Os -finline-limit=64
  ESCARGOT_CXXFLAGS_RELEASE += -UUSE_ES6_FEATURE
  ifeq ($(HOST),tizen_obs)
    ESCARGOT_CXXFLAGS_DEBUG += -O1 # _FORTIFY_SOURCE requires compiling with optimization
  endif
endif

#######################################################
# flags for $(OUTPUT) : bin/shared_lib/static_lib
#######################################################
ESCARGOT_CXXFLAGS_BIN += -fvisibility=hidden -DESCARGOT_STANDALONE
ESCARGOT_LDFLAGS_BIN += -Wl,--gc-sections

ESCARGOT_CXXFLAGS_SHAREDLIB += -fPIC
ESCARGOT_LDFLAGS_SHAREDLIB += -ldl

ESCARGOT_CXXFLAGS_STATICLIB += -fPIC
ESCARGOT_LDFLAGS_STATICLIB += -Wl,--gc-sections

#######################################################
# flags for LTO
#######################################################
ESCARGOT_CXXFLAGS_LTO += -flto -ffat-lto-objects
ESCARGOT_LDFLAGS_LTO += -flto

#######################################################
# flags for $(THIRD_PARTY)
#######################################################
# icu
ifeq ($(HOST), linux)
  ifeq ($(ARCH), x64)
	ESCARGOT_CXXFLAGS_THIRD_PARTY += $(shell pkg-config --cflags icu-i18n)
	ESCARGOT_LDFLAGS_THIRD_PARTY += $(shell pkg-config --libs icu-i18n)
  else ifeq ($(ARCH), x86)
	ESCARGOT_CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/deps/x86-linux/include
	ESCARGOT_LDFLAGS_THIRD_PARTY += -Ldeps/x86-linux/lib
	ESCARGOT_LDFLAGS_THIRD_PARTY += -licuio -licui18n -licuuc -licudata
  endif
else ifneq (,$(findstring tizen_,$(HOST)))
  ifeq ($(ARCH), arm)
	ESCARGOT_CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/deps/tizen/include
	ESCARGOT_LDFLAGS_THIRD_PARTY += -Ldeps/tizen/lib/tizen-wearable-$(VERSION)-target-arm
	ESCARGOT_LDFLAGS_THIRD_PARTY += -licuio -licui18n -licuuc -licudata
  else ifeq ($(ARCH), i386)
	ESCARGOT_CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/deps/tizen/include
	ESCARGOT_LDFLAGS_THIRD_PARTY += -Ldeps/tizen/lib/tizen-wearable-$(VERSION)-emulator-x86
	ESCARGOT_LDFLAGS_THIRD_PARTY += -licuio -licui18n -licuuc -licudata
  endif
endif

# bdwgc
ESCARGOT_CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/third_party/bdwgc/include/
ifeq ($(MODE), debug)
  ESCARGOT_CXXFLAGS_THIRD_PARTY += -DGC_DEBUG
endif

# netlib
ESCARGOT_CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/third_party/netlib/

# v8's fast-dtoa
ESCARGOT_CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/third_party/double_conversion/

# rapidjson
ESCARGOT_CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/third_party/rapidjson/include/

# yarr
ESCARGOT_CXXFLAGS_THIRD_PARTY += -I$(ESCARGOT_ROOT)/third_party/yarr/

#######################################################
# for printing TC coverage log
#######################################################
ESCARGOT_CXXFLAGS_TC = -DSTARFISH_TC_COVERAGE

