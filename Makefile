BIN=escargot

#######################################################
# Environments
#######################################################

HOST=linux
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

ifeq ($(HOST), linux)
	CC = gcc
	CXX = g++
	CXXFLAGS = -std=c++11
endif

$(info goal... $(MAKECMDGOALS))

ifneq (,$(findstring x86,$(MAKECMDGOALS)))
	ARCH=x86
else ifneq (,$(findstring x64,$(MAKECMDGOALS)))
	ARCH=x64
endif

ifneq (,$(findstring interpreter,$(MAKECMDGOALS)))
	TYPE=interpreter
else ifneq (,$(findstring jit,$(MAKECMDGOALS)))
	ifneq ($(MAKECMDGOALS), check-jit)
		TYPE=jit
	endif
endif

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
	MODE=debug
else ifneq (,$(findstring release,$(MAKECMDGOALS)))
	MODE=release
endif

BUILDDIR=out/$(ARCH)/$(TYPE)/$(MODE)

$(info host... $(HOST))
$(info arch... $(ARCH))
$(info type... $(TYPE))
$(info mode... $(MODE))
$(info build dir... $(BUILDDIR))

ifeq ($(TYPE), intrepreter)
	CXXFLAGS+=$(CXXFLAGS_INTERPRETER)
else ifeq ($(TYPE), jit)
	CXXFLAGS+=$(CXXFLAGS_JIT)
endif

ifeq ($(ARCH), x64)
	CXXFLAGS += -DESCARGOT_64=1
else ifeq ($(ARCH), x86)
	#https://gcc.gnu.org/onlinedocs/gcc-4.8.0/gcc/i386-and-x86_002d64-Options.html
	CXXFLAGS += -DESCARGOT_32=1 -m32  -march=native -mtune=native -mfpmath=sse -msse2 -msse3
	LDFLAGS += -m32
endif

ifeq ($(MODE), debug)
	CXXFLAGS += $(CXXFLAGS_DEBUG)
else ifeq ($(MODE), release)
	CXXFLAGS += $(CXXFLAGS_RELEASE)
endif


#######################################################
# Global build flags
#######################################################

# common flags
CXXFLAGS += -fno-rtti -fno-math-errno -Isrc/
CXXFLAGS += -fdata-sections -ffunction-sections

LDFLAGS += -lpthread
# -ltcmalloc_minimal
LDFLAGS += -Wl,--gc-sections

# flags for debug/release
CXXFLAGS_DEBUG = -O0 -g3 -D_GLIBCXX_DEBUG -frounding-math -fsignaling-nans -fno-omit-frame-pointer -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-invalid-offsetof -Wno-sign-compare -Wno-unused-local-typedefs
CXXFLAGS_RELEASE = -O2 -g3 -DNDEBUG -fomit-frame-pointer -frounding-math -fsignaling-nans

# flags for jit/interpreter
CXXFLAGS_JIT = -DENABLE_ESJIT=1
CXXFLAGS_INTERPRETER =

#######################################################
# Third-party build flags
#######################################################

# bdwgc
CXXFLAGS += -Ithird_party/bdwgc/include/
#CXXFLAGS_DEBUG += -DGC_DEBUG
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
		CXXFLAGS += -DAVMPLUS_64BIT
		CXXFLAGS += -DAVMPLUS_AMD64
		CXXFLAGS += -DAVMPLUS_X64
		CXXFLAGS += #if defined(_M_AMD64) || defined(_M_X64)
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
endif

# netlib
CXXFLAGS += -Ithird_party/netlib/

# v8's fast-dtoa
CXXFLAGS += -Ithird_party/double_conversion/
SRC_DTOA =
SRC_DTOA += $(foreach dir, ./third_party/double_conversion , $(wildcard $(dir)/*.cc))

# rapidjson
CXXFLAGS += -Ithird_party/rapidjson/include/

# yarr
CXXFLAGS += -Ithird_party/yarr/
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
SRC += $(foreach dir, ./src , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/ast , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/bytecode , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/jit , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/parser , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/runtime , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/shell , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/vm , $(wildcard $(dir)/*.cpp))

SRC += $(SRC_YARR)
SRC += $(SRC_ESPRIMA_CPP)
ifeq ($(TYPE), jit)
	SRC += $(SRC_NANOJIT)
endif

SRC_CC =
SRC_CC += $(SRC_DTOA)

OBJS := $(SRC:%.cpp= $(BUILDDIR)/%.o)
OBJS += $(SRC_CC:%.cc= $(BUILDDIR)/%.o)
OBJS += $(SRC_C:%.c= $(BUILDDIR)/%.o)

#######################################################
# Targets
#######################################################

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

.DEFAULT_GOAL:=x64.jit.debug

#x86.jit.debug: $(BUILDDIR)/$(BIN)
#	cp -f $< .
#x86.jit.release: $(BUILDDIR)/$(BIN)
#	cp -f $< .
x86.interpreter.debug: $(BUILDDIR)/$(BIN)
	cp -f $< .
x86.interpreter.release: $(BUILDDIR)/$(BIN)
	cp -f $< .
x64.jit.debug: $(BUILDDIR)/$(BIN)
	cp -f $< .
x64.jit.release: $(BUILDDIR)/$(BIN)
	cp -f $< .
x64.interpreter.debug: $(BUILDDIR)/$(BIN)
	cp -f $< .
x64.interpreter.release: $(BUILDDIR)/$(BIN)
	cp -f $< .

$(BUILDDIR)/$(BIN): $(OBJS) $(THIRD_PARTY_LIBS)
	$(CXX) -o $@ $(OBJS) $(THIRD_PARTY_LIBS) $(LDFLAGS)

$(BUILDDIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) $< -o $@
	$(CXX) -MM $(CXXFLAGS) -MT $@ $< > $(BUILDDIR)/$*.d
	
$(BUILDDIR)/%.o: %.cc
	mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) $< -o $@
	$(CXX) -MM $(CXXFLAGS) -MT $@ $< > $(BUILDDIR)/$*.d

$(BUILDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@
	$(CC) -MM $(CFLAGS) -MT $@ $< > $(BUILDDIR)/$*.d

full:
	make x64.jit.debug -j$(NPROCS)
	ln -sf out/x64/jit/debug/$(BIN) $(BIN).x64.jd
	make x64.jit.release -j$(NPROCS)
	ln -sf out/x64/jit/release/$(BIN) $(BIN).x64.jr
	make x64.interpreter.debug -j$(NPROCS)
	ln -sf out/x64/interpreter/debug/$(BIN) $(BIN).x64.id
	make x64.interpreter.release -j$(NPROCS)
	ln -sf out/x64/interpreter/release/$(BIN) $(BIN).x64.ir

# Targets : miscellaneous

clean:
	rm -rf out

strip:
	strip $(BIN)

asm:
	objdump -d        $(BIN) | c++filt > $(BIN).asm
	readelf -a --wide $(BIN) | c++filt > $(BIN).elf
	vi -O $(BIN).asm $(BIN).elf

# Targets : tests

check-jit:
	make x64.jit.release -j$(NPROCS)
	make run-sunspider
	make x64.interpreter.release -j$(NPROCS)
	make run-sunspider
	make x64.jit.debug -j$(NPROCS)
	./run-Sunspider-jit.sh -rcf > compiledFunctions.txt
	vimdiff compiledFunctions.txt originalCompiledFunctions.txt
	./run-Sunspider-jit.sh -rof > osrExitedFunctions.txt
	vimdiff osrExitedFunctions.txt originalOSRExitedFunctions.txt

check:
	make x64.interpreter.release -j$(NPROCS)
	make run-sunspider | tee out/sunspider_result
	make run-octane > out/octane_result
	make x64.interpreter.debug -j$(NPROCS)
	make run-sunspider
	make check-jit
	cat out/sunspider_result
	cat out/octane_result
	./regression_test262
	make tidy

tidy:
	./tools/check-webkit-style `find src/ -name "*.cpp" -o -name "*.h"`> error_report 2>& 1

# Targets : benchmarks

run-sunspider:
	cd test/SunSpider/; \
	./sunspider --shell=../../escargot --suite=sunspider-1.0.2

run-octane:
	cd test/octane/; \
	../../escargot run_escargot_test.js

run-test262:
	cd test/test262/; \
	python tools/packaging/test262.py --command ../../escargot $(OPT) --summary

.PHONY: clean
