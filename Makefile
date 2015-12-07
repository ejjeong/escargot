BUILDDIR=./build
HOST=linux
include $(BUILDDIR)/Toolchain.mk

BIN=escargot

#######################################################
# Environments
#######################################################

ARCH=#x86,x64
TYPE=none#interpreter
MODE=#debug,release
NPROCS:=1
OS:=$(shell uname -s)
SHELL:=/bin/bash
ifeq ($(OS),Linux)
  NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
  SHELL:=/bin/bash
endif
ifeq ($(OS),Darwin)
  NPROCS:=$(shell sysctl -n machdep.cpu.thread_count)
  SHELL:=/opt/local/bin/bash
endif

$(info goal... $(MAKECMDGOALS))

ifneq (,$(findstring x86,$(MAKECMDGOALS)))
  ARCH=x86
else ifneq (,$(findstring x64,$(MAKECMDGOALS)))
  ARCH=x64
else ifneq (,$(findstring arm,$(MAKECMDGOALS)))
  ARCH=arm
endif

ifneq (,$(findstring interpreter,$(MAKECMDGOALS)))
  TYPE=interpreter
else ifneq (,$(findstring jit,$(MAKECMDGOALS)))
  ifeq (,$(findstring check-jit,$(MAKECMDGOALS)))
    TYPE=jit
  endif
endif

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
  MODE=debug
else ifneq (,$(findstring release,$(MAKECMDGOALS)))
  MODE=release
endif

ifeq ($(HOST), linux)
  OUTDIR=out/$(ARCH)/$(TYPE)/$(MODE)
else ifeq ($(HOST), tizen)
  OUTDIR=out/tizen_$(ARCH)/$(TYPE)/$(MODE)
endif

$(info host... $(HOST))
$(info arch... $(ARCH))
$(info type... $(TYPE))
$(info mode... $(MODE))
$(info build dir... $(OUTDIR))

ifeq ($(TYPE), intrepreter)
  CPPFLAGS+=$(CXXFLAGS_INTERPRETER)
else ifeq ($(TYPE), jit)
  CPPFLAGS+=$(CXXFLAGS_JIT)
endif

ifeq ($(ARCH), x64)
  CPPFLAGS += -DESCARGOT_64=1
else ifeq ($(ARCH), x86)
  #https://gcc.gnu.org/onlinedocs/gcc-4.8.0/gcc/i386-and-x86_002d64-Options.html
  CPPFLAGS += -DESCARGOT_32=1 -m32  -march=native -mtune=native -mfpmath=sse -msse2 -msse3
  LDFLAGS += -m32
else ifeq ($(ARCH), arm)
  CPPFLAGS += -DESCARGOT_32=1 -march=armv7-a
else
CPPFLAGS += -DESCARGOT_32=1 -march=armv7-a
endif

ifeq ($(MODE), debug)
  CPPFLAGS += $(CXXFLAGS_DEBUG)
else ifeq ($(MODE), release)
  CPPFLAGS += $(CXXFLAGS_RELEASE)
endif


#######################################################
# Global build flags
#######################################################

# common flags
CXXFLAGS += -fno-rtti -fno-math-errno -Isrc/
CXXFLAGS += -fdata-sections -ffunction-sections
CXXFLAGS += -frounding-math -fsignaling-nans
CXXFLAGS += -Wno-invalid-offsetof #-fvisibility=hidden

ifeq ($(HOST), tizen)
  CPPFLAGS += --sysroot=$(TIZEN_SYSROOT)
endif

# Shared Library
SONAME  = libescargot.so
CPPFLAGS += -fPIC -DHAVE_CONFIG_H -Ithird_party/bdwgc/libatomic_ops/src -Ithird_party/bdwgc/out/$(ARCH)/$(MODE)/include
SRC_C += $(foreach dir, third_party/bdwgc , $(wildcard $(dir)/*.c))

LDFLAGS += -lpthread -ldl
# -ltcmalloc_minimal
#LDFLAGS += -Wl,--gc-sections

ifeq ($(HOST), tizen)
  LDFLAGS += --sysroot=$(TIZEN_SYSROOT)
endif

# flags for debug/release
CPPFLAGS_DEBUG = -O0 -g3 -D_GLIBCXX_DEBUG -fno-omit-frame-pointer -Wall -Wextra -Werror
CPPFLAGS_DEBUG += -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -Wno-unused-parameter
CPPFLAGS_RELEASE = -O2 -g3 -DNDEBUG -fomit-frame-pointer -fno-stack-protector -funswitch-loops -Wno-deprecated-declarations

# flags for jit/interpreter
CPPFLAGS_JIT = -DENABLE_ESJIT=1
CPPFLAGS_INTERPRETER =

#######################################################
# Third-party build flags
#######################################################

# bdwgc
CPPFLAGS += -Ithird_party/bdwgc/include/
CPPFLAGS_DEBUG += -DGC_DEBUG
GCLIBS=third_party/bdwgc/out/$(ARCH)/$(MODE)/.libs/libgc.a

ifneq ($(TYPE),none)

ifneq ($(wildcard $(GCLIBS)),)

else
$(info gclib not exists. execute build_third_party...)
$(shell ./build_third_party.sh)
endif

endif

ifeq ($(TYPE), jit)
  #include third_party/nanojit/Build.mk
  ####################################
  # ARCH-dependent settings
  ####################################
  ifeq ($(ARCH), x64)
    TARGET_CPU=x86_64
    CPPFLAGS += -DAVMPLUS_64BIT
    CPPFLAGS += -DAVMPLUS_AMD64
    CPPFLAGS += #if defined(_M_AMD64) || defined(_M_X64)
  else ifeq ($(ARCH), x86)
    TARGET_CPU=i686
    CPPFLAGS += -DAVMPLUS_32BIT
    CPPFLAGS += -DAVMPLUS_IA32
    CPPFLAGS += #if defined(_M_AMD64) || defined(_M_X64)
  else ifeq ($(ARCH), arm)
    TARGET_CPU=arm
    # CPPFLAGS += -mfpu=neon #enabled by LOCAL_ARM_NEON := true
    CPPFLAGS += -DAVMPLUS_32BIT
    CPPFLAGS += -DAVMPLUS_ARM
    CPPFLAGS += -DTARGET_THUMB2
    CPPFLAGS += #if defined(_M_AMD64) || defined(_M_X64)
    SRCS += $(SRC_THIRD_PARTY)/nanojit/NativeARM.cpp
    SRCS += $(SRC_THIRD_PARTY)/nanojit/NativeThumb2.cpp
  endif
  ####################################
  # target-dependent settings
  ####################################
 
  ifeq ($(MODE), debug)
    CPPFLAGS += -DDEBUG
    CPPFLAGS += -D_DEBUG
    CPPFLAGS += -DNJ_VERBOSE
  endif

  ####################################
  # Other features
  ####################################
  CPPFLAGS += -DESCARGOT
  CPPFLAGS += -Ithird_party/nanojit/
  CPPFLAGS += -DFEATURE_NANOJIT

  #CPPFLAGS += -DAVMPLUS_VERBOSE
  CPPFLAGS += -Wno-error=narrowing

  ####################################
  # Makefile flags
  ####################################
  curdir=third_party/nanojit
  include $(curdir)/manifest.mk
  SRC_NANOJIT = $(avmplus_CXXSRCS)
  SRC_NANOJIT += $(curdir)/EscargotBridge.cpp
endif

# netlib
CPPFLAGS += -Ithird_party/netlib/

# v8's fast-dtoa
CPPFLAGS += -Ithird_party/double_conversion/
SRC_DTOA =
SRC_DTOA += $(foreach dir, third_party/double_conversion , $(wildcard $(dir)/*.cc))

# rapidjson
CPPFLAGS += -Ithird_party/rapidjson/include/

# yarr
CPPFLAGS += -Ithird_party/yarr/
SRC_YARR += third_party/yarr/OSAllocatorPosix.cpp
SRC_YARR += third_party/yarr/PageBlock.cpp
SRC_YARR += third_party/yarr/YarrCanonicalizeUCS2.cpp
SRC_YARR += third_party/yarr/YarrInterpreter.cpp
SRC_YARR += third_party/yarr/YarrPattern.cpp
SRC_YARR += third_party/yarr/YarrSyntaxChecker.cpp

# Common
THIRD_PARTY_LIBS= $(GCLIBS)

#######################################################
# SRCS & OBJS
#######################################################

SRC=
SRC += $(foreach dir, src , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/ast , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/bytecode , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/jit , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/parser , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/runtime , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/shell , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/vm , $(wildcard $(dir)/*.cpp))

SRC += $(SRC_YARR)
SRC += $(SRC_ESPRIMA_CPP)
ifeq ($(TYPE), jit)
  SRC += $(SRC_NANOJIT)
endif

SRC_CC =
SRC_CC += $(SRC_DTOA)

OBJS := $(SRC:%.cpp= $(OUTDIR)/%.o)
OBJS += $(SRC_CC:%.cc= $(OUTDIR)/%.o)
OBJS += $(SRC_C:%.c= $(OUTDIR)/%.o)

#######################################################
# Targets
#######################################################

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

.DEFAULT_GOAL:=x64.jit.debug

x86.jit.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
x86.jit.release: $(OUTDIR)/$(BIN)
	cp -f $< .
x86.interpreter.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
x86.interpreter.release: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.jit.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.jit.release: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.interpreter.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.interpreter.release: $(OUTDIR)/$(BIN)
	cp -f $< .
arm.jit.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
arm.jit.release: $(OUTDIR)/$(BIN)
	cp -f $< .
arm.interpreter.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
arm.interpreter.release: $(OUTDIR)/$(BIN)
	cp -f $< .
arm.interpreter.release.shared: $(OBJS)
	$(CXX) -shared -Wl,-soname,$(SONAME) -o $(SONAME) $(OBJS) $(LDFLAGS)

$(OUTDIR)/$(BIN): $(OBJS) $(THIRD_PARTY_LIBS)
	@echo "[LINK] $@"
	@$(CXX) -o $@ $(OBJS) $(THIRD_PARTY_LIBS) $(LDFLAGS)

$(OUTDIR)/%.o: %.cpp Makefile
	@echo "[CXX] $@"
	@mkdir -p $(dir $@)
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
	@$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) -MT $@ $< > $(OUTDIR)/$*.d
	
$(OUTDIR)/%.o: %.cc Makefile
	@echo "[CXX] $@"
	@mkdir -p $(dir $@)
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
	@$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) -MT $@ $< > $(OUTDIR)/$*.d

$(OUTDIR)/%.o: %.c Makefile
	@echo "[CC] $@"
	@mkdir -p $(dir $@)
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@
	@$(CC) -MM $(CPPFLAGS) $(CFLAGS) -MT $@ $< > $(OUTDIR)/$*.d

full:
	make x64.jit.debug -j$(NPROCS)
	ln -sf out/x64/jit/debug/$(BIN) $(BIN).x64.jd
	make x64.jit.release -j$(NPROCS)
	ln -sf out/x64/jit/release/$(BIN) $(BIN).x64.jr
	make x64.interpreter.debug -j$(NPROCS)
	ln -sf out/x64/interpreter/debug/$(BIN) $(BIN).x64.id
	make x64.interpreter.release -j$(NPROCS)
	ln -sf out/x64/interpreter/release/$(BIN) $(BIN).x64.ir
	make x86.jit.debug -j$(NPROCS)
	ln -sf out/x86/jit/debug/$(BIN) $(BIN).x86.jd
	make x86.jit.release -j$(NPROCS)
	ln -sf out/x86/jit/release/$(BIN) $(BIN).x86.jr
	make x86.interpreter.debug -j$(NPROCS)
	ln -sf out/x86/interpreter/debug/$(BIN) $(BIN).x86.id
	make x86.interpreter.release -j$(NPROCS)
	ln -sf out/x86/interpreter/release/$(BIN) $(BIN).x86.ir

# Targets : miscellaneous

clean:
	rm -rf out

strip:
	strip $(BIN)

asm:
	objdump -d        $(BIN) | c++filt > $(BIN).asm
	readelf -a --wide $(BIN) | c++filt > $(BIN).elf
	vi -O $(BIN).asm $(BIN).elf

# Targets : Regression tests

check-jit-64:
	make x64.jit.release -j$(NPROCS)
	make run-sunspider
	make run-octane
	make x64.jit.debug -j$(NPROCS)
	./run-Sunspider.sh -rcf > compiledFunctions.txt
	vimdiff compiledFunctions.txt originalCompiledFunctions.txt
	./run-Sunspider.sh -rof > osrExitedFunctions.txt
	vimdiff osrExitedFunctions.txt originalOSRExitedFunctions.txt

check-jit-32:
	make x86.jit.release -j$(NPROCS)
	make run-sunspider
	make run-octane
	make x86.jit.debug -j$(NPROCS)
	./run-Sunspider.sh -rcf > compiledFunctions.txt
	vimdiff compiledFunctions.txt originalCompiledFunctions.txt
	./run-Sunspider.sh -rof > osrExitedFunctions.txt
	vimdiff osrExitedFunctions.txt originalOSRExitedFunctions.txt

check-jit-arm:
	./setup_measure_for_android.sh build-jit
	#./measure_for_android.sh escargot32.jit time > time.arm32.txt
	adb shell "cd /data/local/tmp ; ./run-Sunspider.sh /data/local/tmp/arm32/escargot/jit/escargot.debug -rcf > compiledFunctions.arm32.txt"
	adb shell "cd /data/local/tmp ; ./run-Sunspider.sh /data/local/tmp/arm32/escargot/jit/escargot.debug -rof > osrExitedFunctions.arm32.txt"
	#adb pull /data/local/tmp/time.arm32.txt .
	adb pull /data/local/tmp/compiledFunctions.arm32.txt .
	adb pull /data/local/tmp/osrExitedFunctions.arm32.txt .

check:
	make x64.interpreter.release -j$(NPROCS)
	make run-sunspider | tee out/sunspider_result
	make run-octane | tee out/octane_result
	make x64.interpreter.debug -j$(NPROCS)
	make run-sunspider
	make check-jit-64
	cat out/sunspider_result
	cat out/octane_result
	./regression_test262
	make tidy

check-lirasm:
	make x64.jit.debug -j8; \
	cd test/lirasm; \
	./testlirc.sh ../../escargot; \
	cd ../..; \
	make x86.jit.debug -j8; \
	cd test/lirasm; \
	./testlirc.sh ../../escargot;

check-lirasm-android:
	adb shell su -e mkdir -p /data/local/tmp/lirasm/tests
	adb push ./android/libs/armeabi-v7a/escargot /data/local/tmp/lirasm/escargot
	adb push test/lirasm/tests /data/local/tmp/lirasm/tests/
	cd test/lirasm/; ./testlirc_android.sh

tidy:
	./tools/check-webkit-style `find src/ -name "*.cpp" -o -name "*.h"`> error_report 2>& 1

# Targets : benchmarks

run-sunspider:
	cd test/SunSpider/; \
	./sunspider --shell=../../escargot --suite=sunspider-1.0.2

run-octane:
	cd test/octane/; \
	../../escargot run.js

run-test262:
	cd test/test262/; \
	python tools/packaging/test262.py --command ../../escargot $(OPT) --summary

.PHONY: clean
